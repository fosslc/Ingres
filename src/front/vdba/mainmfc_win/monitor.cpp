/********************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : monitor.cpp
**    interface functions with low-level for monitoring
**
** History after 01-01-2000:
**   20-Jan-2000 (uk$so01)
**    (Bug #100063)  Eliminate the undesired compiler's warning
**   26-Jan-2000 (noifr01)
**    (bug 100155) internal comparisions that were done on a session id
**    are now done both on the session id and the server pid
**   03-Feb-2000 (noifr01)
**    (SIR 100331) manage the (new) RMCMD monitor server type
**   06-Dec-2000 (schph01)
**    (SIR 102822) save the current "lgd_end" and "lgd_writeio" into
**    "lgd_Present_end", "lgd_Present_writeio" because "lgd_end" and
**    "lgd_writeio" are used to calculate the difference between the
**     start time.
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   27-Jul-2001 (noifr01)
**    (bug 102895) take into account the "capabilities" of the server
**    (now queried in ima_dbms_server_parameters) in the 
**    FindClassServerType() function
**   27-Jul-2001 (noifr01)
**     (sir 105275) misc cleanup for the display of servers in monitor
**     windows
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**   07-Jan-2003 (noifr01 and schph01)
**     (bug 109341) parenthood was not passed correctly in GetFirstMonInfo
**     for the "Opened Windows" sub-branch under servers/user in the nodes
**     window
** 22-Sep-2004 (uk$so01)
**    BUG #113106
**    Open DOM Windows on the remote virtual node through Nodes/Servers/Users causes 
**    an error with "System error 8" message to be displayed.
********************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "extccall.h"

extern "C" {
#include "monitor.h"
#include "dba.h"
#include "domdata.h"
#include "main.h"
#include "getdata.h"
#include "error.h"
#include "dbaginfo.h"
#include "tools.h"
#include "dbaset.h"
};

STYPELOCK Type_Lock [] = {
    RESTYPE_ALL           , "Unknown",
    RESTYPE_DATABASE      , "Database",
    RESTYPE_TABLE         , "Table",
    RESTYPE_PAGE          , "Page",
    RESTYPE_EXTEND_FILE   , "Extend",
    RESTYPE_BM_PAGE       , "SV Page",
    RESTYPE_CREATE_TABLE  , "Create Table",
    RESTYPE_OWNER_ID      , "Owner ID",
    RESTYPE_CONFIG        , "Config",
    RESTYPE_DB_TEMP_ID    , "Temp ID",
    RESTYPE_SV_DATABASE   , "SV Database",
    RESTYPE_SV_TABLE      , "SV Table",
    RESTYPE_SS_EVENT      , "SV Event",
    RESTYPE_TBL_CONTROL   , "Table Control",
    RESTYPE_JOURNAL       , "Journal",
    RESTYPE_OPEN_DB       , "Open DB",
    RESTYPE_CKP_DB        , "CKP DB",
    RESTYPE_CKP_CLUSTER   , "CKP Cluster",
    RESTYPE_BM_LOCK       , "BM Lock",
    RESTYPE_BM_DATABASE   , "BM Database",
    RESTYPE_BM_TABLE      , "BM Table",
    RESTYPE_CONTROL       , "Control",
    RESTYPE_EVCONNECT     , "Event Connect",
    RESTYPE_AUDIT         , "Audit",
    RESTYPE_ROW           , "Row",
    -1                    , NULL
                 };

struct ClassServerStruct {
         CHAR  *lpValues[3];
         int nVal;
          } ClassServerKnown[] = {
                        {"IINMSVR","NAME"     ,"NAME"     ,SVR_TYPE_GCN     },  
                        {"IUSVR"  ,"RECOVERY" ,"RECOVERY" ,SVR_TYPE_RECOVERY},
                        {"INGRES" ,"DBMS"     ,"INGRES"   ,SVR_TYPE_DBMS    },
                        {"COMSVR" ,"NET"      ,"NET"      ,SVR_TYPE_NET     },
                        {"STAR"   ,"STAR"     ,"STAR"     ,SVR_TYPE_STAR    },
                        {""       ,"OTHER"    ,"OTHER"    ,SVR_TYPE_OTHER   },
						{"ICESVR" ,"ICE"      ,"ICE"      ,SVR_TYPE_ICE     },
						{"RMCMD"  ,"RMCMD"    ,"RMCMD"    ,SVR_TYPE_RMCMD   },
						{"JDBC"   ,"JDBC"     ,"JDBC"     ,SVR_TYPE_JDBC   },
                        {NULL,"\0","\0", 0}};

static char * GetMonDbNameFromKey(char * lpkey)
{
   static char dbname[100];
   char * pc;

   pc = x_strrchr(lpkey,',');
   if (!pc)
     return NULL;
   x_strcpy(dbname,pc+1);
   pc=x_strrchr(dbname,')');
   if (!pc)
     return NULL;
   *pc='\0';

   return dbname;
}

static int GetDBTypeFromKey(int hdl, char * lpkey)
{
   UCHAR buf       [MAXOBJECTNAME];
   UCHAR buffilter [MAXOBJECTNAME];
   UCHAR extradata [EXTRADATASIZE];
   int ires;

   int idbtype=DBTYPE_ERROR;

   char * lpdbname = GetMonDbNameFromKey(lpkey);
   if (!lpdbname)
      return RES_ERR;

   push4domg ();
   ires = DOMGetFirstObject ( hdl, OT_DATABASE, 0, NULL, TRUE, NULL,buf,buffilter, extradata);
   while (ires == RES_SUCCESS) {
      if (!x_stricmp(lpdbname,(char *)buf)) {
         idbtype = getint((LPUCHAR)extradata+STEPSMALLOBJ);
         break;
      }
      ires  = DOMGetNextObject (buf, buffilter, extradata);
   }
   pop4domg ();
   return idbtype;
}

static int GetDBTypeFromName(int hdl, char * lpname)
{
   UCHAR buf       [MAXOBJECTNAME];
   UCHAR buffilter [MAXOBJECTNAME];
   UCHAR extradata [EXTRADATASIZE];
   int ires;

   int idbtype=DBTYPE_ERROR;

   push4domg ();
   ires = DOMGetFirstObject ( hdl, OT_DATABASE, 0, NULL, TRUE, NULL,buf,buffilter, extradata);
   while (ires == RES_SUCCESS) {
      if (!x_stricmp(lpname,(char *)buf)) {
         idbtype = getint((LPUCHAR)extradata+STEPSMALLOBJ);
         break;
      }
      ires  = DOMGetNextObject (buf, buffilter, extradata);
   }
   pop4domg ();
   return idbtype;

}

static char * GetMonDbNameFromID(int hdl, long dbid)
{
static  UCHAR buf  [MAXOBJECTNAME];
   UCHAR buffilter [MAXOBJECTNAME];
   UCHAR extradata [MAXOBJECTNAME];
   char * pres =NULL;
   int ires,db_id;

   push4domg ();
   ires = DOMGetFirstObject ( hdl, OT_DATABASE, 0, NULL, TRUE, NULL,buf,buffilter, extradata);
   while (ires == RES_SUCCESS) {
      db_id = getint((LPUCHAR)extradata);
      if (dbid == db_id) {
         pres= (char *)buf;
         break;
      }
      ires  = DOMGetNextObject (buf, buffilter, extradata);
   }
   pop4domg ();
   return pres;
}

static int GetMonDbID(int hdl, char * lpkey, long * plresult)
{
   UCHAR buf       [MAXOBJECTNAME];
   UCHAR buffilter [MAXOBJECTNAME];
   UCHAR extradata [EXTRADATASIZE];
   int ires;
   char * lpdbname = GetMonDbNameFromKey(lpkey);
   *plresult = 0L;

   if (!lpdbname)
      return RES_ERR;

   push4domg ();
   ires = DOMGetFirstObject ( hdl, OT_DATABASE, 0, NULL, TRUE, NULL,buf,buffilter, extradata);
   while (ires == RES_SUCCESS) {
      if (!x_stricmp(lpdbname,(char *)buf)) {
         *plresult = (long) getint((LPUCHAR)extradata);
         break;
      }
      ires  = DOMGetNextObject (buf, buffilter, extradata);
   }
   pop4domg ();
   return ires;
}

static char * GetMonTableName(int hdl, LPUCHAR lpdatabasename,char *lpresulttbowner,long tbid)
{
   LPUCHAR parentstrings [MAXPLEVEL];
static   char buf       [MAXOBJECTNAME];
   char buffilter [MAXOBJECTNAME];
   char extradata [MAXOBJECTNAME];
   char * pres =NULL;
   int tbl_id,ires;


   parentstrings [0] = (LPUCHAR)lpdatabasename;
   parentstrings [1] = NULL;

   push4domg ();
   ires = DOMGetFirstObject (hdl, OT_TABLE, 1,
                             parentstrings, TRUE, NULL, (LPUCHAR)buf, (LPUCHAR)buffilter,(LPUCHAR) extradata);

   while (ires == RES_SUCCESS)  {
      tbl_id = getint((LPUCHAR)(extradata));
      if (tbl_id == tbid)  {
         pres=buf;
         x_strcpy(lpresulttbowner,buffilter);
         break;
      }
      ires    = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, (LPUCHAR)extradata);
   }
   if (!pres) {
     ires = DOMGetFirstObject (hdl, OT_VIEWALL, 1,
                         parentstrings, TRUE, NULL, (LPUCHAR)buf, (LPUCHAR)buffilter,(LPUCHAR) extradata);
     while (ires == RES_SUCCESS)  {
        tbl_id = getint((LPUCHAR)(extradata+4));
        if (tbl_id == tbid)  {
           pres=buf;
           x_strcpy(lpresulttbowner,buffilter);
           break;
        }
        ires    = DOMGetNextObject ((LPUCHAR)buf, (LPUCHAR)buffilter, (LPUCHAR)extradata);
     }
   }
   pop4domg ();
   return pres;
}

static char * GetMonIndexName(int hdl, LPUCHAR lpdatabasename,LPUCHAR lptblname,LPUCHAR lptblowner,LPUCHAR lpresultidxowner,long idxid)
{
   LPUCHAR parentstrings [MAXPLEVEL];
static   char buf1       [MAXOBJECTNAME];
   char buffilter [MAXOBJECTNAME];
   char extradata [MAXOBJECTNAME];
   char buftemp   [MAXOBJECTNAME];
   char * pres =NULL;
   int ind_id,ires;

   parentstrings [0] = (LPUCHAR)lpdatabasename;
   parentstrings [1] = StringWithOwner(lptblname, lptblowner, (LPUCHAR)buftemp);
   parentstrings [2] = NULL;

   push4domg ();
   ires = DOMGetFirstObject (hdl, OT_INDEX, 2,
                             parentstrings, TRUE, NULL, (LPUCHAR)buf1, (LPUCHAR)buffilter,(LPUCHAR) extradata);

   while (ires == RES_SUCCESS)  {
      ind_id = getint((LPUCHAR)(extradata+24));
      if (ind_id == idxid)  {
         pres=buf1;
         x_strcpy((char *)lpresultidxowner,buffilter);
         break;
      }
      ires    = DOMGetNextObject ((LPUCHAR)buf1, (LPUCHAR)buffilter, (LPUCHAR)extradata);
   }
   pop4domg ();
   return pres;
}
static void lock2res(LPLOCKDATAMIN lplock, LPRESOURCEDATAMIN lpres)
{
  x_strcpy(lpres->resource_key     ,lplock->resource_key);
  x_strcpy(lpres->res_database_name,lplock->res_database_name);
  lpres->resource_id           = lplock->resource_id;
  lpres->resource_grant_mode[0]='\0';
  lpres->resource_convert_mode[0]='\0';
  lpres->resource_type         = lplock->resource_type;
  lpres->resource_database_id  = lplock->resource_database_id;
  lpres->resource_table_id     = lplock->resource_table_id;
  lpres->resource_index_id     = lplock->resource_index_id;
  lpres->resource_page_number  = lplock->resource_page_number;
  lpres->hdl                   = lplock->hdl;
  lpres->resource_row_id       = 0;
}

void FillMonLockDBName(LPLOCKDATAMIN lplock) {
   RESOURCEDATAMIN resdta;
   lock2res(lplock,&resdta);
   FillMonResourceDBName(&resdta);
   x_strcpy(lplock->res_database_name,resdta.res_database_name);
}


void FillMonResourceDBName(LPRESOURCEDATAMIN lpres)
{
  char * pc;
  lpres->res_database_name[0]='\0';
  switch (lpres->resource_type) {
    case RESTYPE_DATABASE:
    case RESTYPE_DB_TEMP_ID :
    case RESTYPE_SV_DATABASE:
    case RESTYPE_JOURNAL    :
    case RESTYPE_OPEN_DB    :
      pc=GetMonDbNameFromKey(lpres->resource_key);
      if (pc)
         x_strcpy(lpres->res_database_name,pc);
      GetMonDbID(lpres->hdl, lpres->resource_key, &(lpres->resource_database_id));
      break;
    case RESTYPE_BM_DATABASE:
    case RESTYPE_TABLE:
    case RESTYPE_PAGE:
    case RESTYPE_BM_PAGE:
    case RESTYPE_BM_TABLE:
    case RESTYPE_SV_TABLE:
    case RESTYPE_EXTEND_FILE:
    case RESTYPE_CREATE_TABLE:
    case RESTYPE_TBL_CONTROL:
    case RESTYPE_CKP_DB:
      pc = GetMonDbNameFromID(lpres->hdl, lpres->resource_database_id);
      if (pc)
         x_strcpy(lpres->res_database_name,pc);
  }
}

static void GetMonResourceTblPg(LPRESOURCEDATAMIN lpres, char * prestbl, char * presidx, long * ptblid,  long * pindexno, long * ppageno)
{
  char bufowner[MAXOBJECTNAME];
  char * pc;
  if (prestbl)
     *prestbl='\0';
  if (presidx)
     *presidx='\0';
  *ppageno  =-1L;
  *ptblid   =-1L;
  *pindexno =-1L;
  switch (lpres->resource_type) {
    case RESTYPE_DATABASE:
    case RESTYPE_DB_TEMP_ID :
    case RESTYPE_SV_DATABASE:
    case RESTYPE_JOURNAL    :
    case RESTYPE_OPEN_DB    :
    case RESTYPE_BM_DATABASE:
    case RESTYPE_CREATE_TABLE:
    case RESTYPE_TBL_CONTROL:
    case RESTYPE_CKP_DB:
      break;
    case RESTYPE_TABLE:
    case RESTYPE_PAGE:
    case RESTYPE_BM_PAGE:
    case RESTYPE_BM_TABLE:
    case RESTYPE_SV_TABLE:
    case RESTYPE_EXTEND_FILE:
      *ptblid = lpres->resource_table_id;
      if (prestbl && lpres->res_database_name[0]) {
         pc = GetMonTableName(lpres->hdl,(LPUCHAR)lpres->res_database_name,
                              bufowner,lpres->resource_table_id);
         if (pc) 
           x_strcpy(prestbl,pc);
         if (lpres->resource_index_id && presidx) {
           pc = GetMonIndexName(lpres->hdl,(LPUCHAR) lpres->res_database_name,
                                (LPUCHAR)prestbl,(LPUCHAR)bufowner,
                                (LPUCHAR)bufowner,lpres->resource_index_id);
           if (pc) 
             x_strcpy(presidx,pc);

         }
      }
      switch(lpres->resource_type) {
         case RESTYPE_PAGE:
         case RESTYPE_BM_PAGE:
            *ppageno = lpres->resource_page_number;
            if (lpres->resource_index_id)
              *pindexno = lpres->resource_index_id;
            break;
      }
      break;
  }
}
static char * FormatResourceDbTblPgInfo(LPRESOURCEDATAMIN lpres, char * bufres)
{
  char tblname[MAXOBJECTNAME];
  char indexname[MAXOBJECTNAME];
  long tbid,pageno,indexid;

  GetMonResourceTblPg(lpres, tblname, indexname, &tbid, &indexid, &pageno);
  bufres[0]='\0';
  if (lpres->res_database_name[0]) {
    wsprintf(bufres,"[%s]",lpres->res_database_name);
    if (tblname[0]) {
      x_strcat(bufres,"[");
      x_strcat(bufres,tblname);
      x_strcat(bufres,"]");
      if (indexname[0]) {
         x_strcat(bufres,"[");
         x_strcat(bufres,indexname);
         x_strcat(bufres,"]");
      }
      if (pageno>=0) {
         wsprintf(tblname,"%ld",pageno);
         x_strcat(bufres,tblname);
      }
    }
  }
  return bufres;
}

char * FormatServerSession(long lValue, char * lpBufRes)
{
   if (lValue == -1) {
      x_strcpy(lpBufRes,VDBA_MfcResourceString (IDS_NOT_AVAILABLE));//"n/a"
      return lpBufRes;
   }
   wsprintf(lpBufRes,"%ld",lValue);
   return lpBufRes;
}
char * FormatStateString ( LPSESSIONDATAMIN pSess ,char * lpBufRes)
{
   if (x_strstr(pSess->session_state,"EVENT_WAIT") != NULL)   {
      x_strcpy(lpBufRes,pSess->session_state);
      x_strcat(lpBufRes," (");
      x_strcat(lpBufRes,pSess->session_wait_reason);
      x_strcat(lpBufRes,")");
      return lpBufRes;
   }
   return pSess->session_state;
}

char * FormatHexaS(char * lpValStr,char * lpBufRes)
{
   long sesid;

   sesid=atol( lpValStr );
   wsprintf(lpBufRes,"%lx",sesid);
   return (lpBufRes);
}

char * FormatHexaL(long lpVal,char * lpBufRes)
{
   wsprintf(lpBufRes,"%lx",lpVal);
   return (lpBufRes);
}
char * FindResourceString(int iTypeRes)
{
   struct type_lock *lptypelock;
   lptypelock = Type_Lock;
   while (lptypelock->iResource != -1)  {
      if ( iTypeRes == lptypelock->iResource)
         return lptypelock->szStringLock;      
   lptypelock++;
   }
   return "Unknown resource string";
}

char * GetLockTableNameStr(LPLOCKDATAMIN lplock, char * bufres, int bufsize)
{
  RESOURCEDATAMIN resdta;
  lock2res(lplock,&resdta);
  return GetResTableNameStr(&resdta,bufres,bufsize);
}

char * GetLockPageNameStr(LPLOCKDATAMIN lplock, char * bufres)
{
  RESOURCEDATAMIN resdta;
  lock2res(lplock,&resdta);
  return GetResPageNameStr(&resdta,bufres);
}

char * GetResTableNameStr(LPRESOURCEDATAMIN lpres, char * bufres, int bufsize)
{
  char tbl    [MAXOBJECTNAME];
  char idxname[MAXOBJECTNAME];
  long tblid, indexno,pageno;
  GetMonResourceTblPg(lpres,tbl,idxname, &tblid,&indexno,&pageno);
  bufres[0]='\0';
  if (tbl[0]) {
    if (idxname[0])  {
      if ((int)(x_strlen(tbl)+x_strlen(idxname)+2 )>=bufsize) {
        if (bufsize>5) {
          x_strcpy(bufres,"[..]");
          bufres+=4;
          bufsize-=4;
        }
        fstrncpy((LPUCHAR)bufres,(LPUCHAR)tbl,bufsize);
      }
      else 
        wsprintf(bufres,"[%s]%s",tbl,idxname);
    }
    else
      fstrncpy((LPUCHAR)bufres,(LPUCHAR)tbl,bufsize);
  }
  return bufres;
}

char * GetResPageNameStr(LPRESOURCEDATAMIN lpres, char * bufres)
{
  switch (lpres->resource_type) {
    case RESTYPE_PAGE:
    case RESTYPE_BM_PAGE:
//      FormatHexaL(lpres->resource_page_number,bufres);
      wsprintf(bufres,"%ld",lpres->resource_page_number);
      break;
    default:
      bufres[0]='\0';
  }
  return bufres;
}

void UpdateMonInfo(int   hnodestruct,
                   int   iobjecttypeParent,
                   void *pstructParent,
                   int   iobjecttypeReq)
{
	NODEDATAMIN nodedata;
	if (iobjecttypeParent == OT_NODE_SERVERCLASS &&
		iobjecttypeReq    == OT_NODE_OPENWIN) {
		NODESVRCLASSDATAMIN svrclass = *((LPNODESVRCLASSDATAMIN)pstructParent);
		int ires;
		BOOL bFound = FALSE;
		push4mong();
		ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
		while (ires==RES_SUCCESS) {
			if (!x_strcmp((char *)nodedata.NodeName,(char *)svrclass.NodeName)) { 
				bFound=TRUE;
				break;
			}
			ires=GetNextMonInfo((void * )&nodedata);
		}
		pop4mong();
		if (!bFound) {
			myerror(ERR_PARENTNOEXIST);
			return ;
		}
		pstructParent=&nodedata; // parent for OT_NODE_OPENWIN must be the node 
	}
	else
	if (iobjecttypeParent == OT_USER && iobjecttypeReq == OT_NODE_OPENWIN) 
	{
		NODEUSERDATAMIN usr = *((LPNODEUSERDATAMIN)pstructParent);
		BOOL bFound = FALSE;
		push4mong();
		int ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
		while (ires==RES_SUCCESS) {
			if (!x_strcmp((char *)nodedata.NodeName,(char *)usr.NodeName)) { 
				bFound=TRUE;
				break;
			}
			ires=GetNextMonInfo((void * )&nodedata);
		}
		pop4mong();
		if (!bFound) {
			myerror(ERR_PARENTNOEXIST);
			return ;
		}
		pstructParent=&nodedata; // parent for OT_NODE_OPENWIN must be the node 
	}

	UpdateDOMData(hnodestruct,
					iobjecttypeReq,
					0,
					(LPUCHAR *)pstructParent,
					TRUE,
					FALSE);
	return;
}

static SFILTER    sfilterALL = {TRUE,TRUE,TRUE,TRUE,RESTYPE_ALL};  
// functions allowing imbedded GetFirstMonInfo - GetNextMonInfo loops

static int          hnodemem;
static int          iparenttypemem;
static UNIONDATAMIN structparentmem;
static int          itypemem;
static SFILTER      sfiltermem;
static BOOL         bstructparentmem;
#define MAXLOOP 8

struct type_stack4mong {
 int          hnodemem;
 int          iparenttypemem;
 UNIONDATAMIN structparentmem;
 int          itypemem;
 SFILTER      sfiltermem;
 BOOL         bstructparentmem;
} stack4mong[MAXLOOP];

static int monstacklevel = 0;

int getMonStack()
{
  return monstacklevel;
}

void push4mong()
{
   if (monstacklevel>=MAXLOOP) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   stack4mong[monstacklevel].hnodemem        =   hnodemem;
   stack4mong[monstacklevel].iparenttypemem  =   iparenttypemem;
   stack4mong[monstacklevel].itypemem        =   itypemem;
   stack4mong[monstacklevel].sfiltermem      =   sfiltermem;
   stack4mong[monstacklevel].bstructparentmem=   bstructparentmem;
   memcpy((char *) &(stack4mong[monstacklevel].structparentmem),
          (char *) &structparentmem,
          sizeof(structparentmem));
   push4domg ();
   monstacklevel++;
}

void pop4mong()
{
   monstacklevel--;
   if (monstacklevel<0) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   hnodemem        =   stack4mong[monstacklevel].hnodemem;
   iparenttypemem  =   stack4mong[monstacklevel].iparenttypemem;
   itypemem        =   stack4mong[monstacklevel].itypemem;
   sfiltermem      =   stack4mong[monstacklevel].sfiltermem;
   bstructparentmem=   stack4mong[monstacklevel].bstructparentmem;
   memcpy((char *) &structparentmem,
          (char *) &(stack4mong[monstacklevel].structparentmem),
          sizeof(structparentmem));
   pop4domg ();
}

BOOL InstallPasswordExists()
{
   NODEDATAMIN nodedata;
   int ires;
   BOOL bResult = FALSE;

   push4mong();
   
   ires=GetFirstMonInfo(0,0,NULL,OT_NODE,(void * )&nodedata,NULL);
   while (ires==RES_SUCCESS) {
       if (nodedata.bInstallPassword) { 
		   bResult=TRUE;
		   break;
	   }
        
       ires=GetNextMonInfo((void * )&nodedata);
   }
   pop4mong();

   return bResult;
}

char * GetTransactionString(int hdl, long tx_high, long tx_low, char * bufres, int bufsize)
{
   LOGTRANSACTDATAMIN transdata;
   int ires;
   push4mong();
   bufres[0]='\0';
   ires=GetFirstMonInfo(hdl,0,NULL,OT_MON_TRANSACTION,&transdata,&sfilterALL);
   while (ires==RES_SUCCESS) {
      if (transdata.tx_transaction_high==tx_high &&
          transdata.tx_transaction_low ==tx_low     ) {
            suppspace((LPUCHAR)(transdata.tx_transaction_id));
            if ((int)x_strlen(transdata.tx_transaction_id)>bufsize-1)
              fstrncpy((LPUCHAR)bufres, (LPUCHAR)VDBA_MfcResourceString(IDS_ERROR), bufsize);//"<error>"
            else
              x_strcpy(bufres,transdata.tx_transaction_id);
         break;
      }
      ires=GetNextMonInfo(&transdata);
   }
   pop4mong();
   return bufres;
}

static BOOL ResTypeOK(int ibranchtype, int llrestype)
{
  BOOL bRes = FALSE;
  switch (llrestype) {
     case RESTYPE_DATABASE:
       if (ibranchtype==OT_MON_RES_DB)
          bRes=TRUE;
       break;
     case RESTYPE_TABLE:
       if (ibranchtype==OT_MON_RES_TABLE)
          bRes=TRUE;
       break;
     case RESTYPE_PAGE:
       if (ibranchtype==OT_MON_RES_PAGE)
          bRes=TRUE;
       break;
     default:
       if (ibranchtype==OT_MON_RES_OTHER)
          bRes=TRUE;
       break;
  }
  return bRes;
}
static int DB2Resource(int hdl,LPRESOURCEDATAMIN lpres)
{
  RESOURCEDATAMIN resdta;
  char DBName[MAXOBJECTNAME];
  int ires;
  char * pc =GetMonDbNameFromKey(lpres->resource_key);
  if (!pc)
    return RES_ERR;
  x_strcpy(DBName,pc);

  push4mong();
  ires=GetFirstMonInfo(hnodemem,0,NULL,
                     OTLL_MON_RESOURCE,&resdta,NULL);
  while (ires==RES_SUCCESS) {
     if (resdta.resource_type==RESTYPE_DATABASE){
        pc =GetMonDbNameFromKey(resdta.resource_key);
        if (!pc) {
          ires=RES_ERR;
          break;
        }
        if (!x_stricmp(pc,DBName)) {
          *lpres=resdta;
          break;
        }
     }
     ires=GetNextMonInfo(&resdta);
  }
  pop4mong();
  return ires;
}
static int FillDBId(int hdl,LPRESOURCEDATAMIN lpres)
{
  long DBid;
  int ires= GetMonDbID(hdl, lpres->resource_key,&DBid);
  if (ires!=RES_SUCCESS)
    return ires;
  lpres->resource_database_id=DBid;
  return RES_SUCCESS;
}

static BOOL ResFilterOK(void * pstructReq,LPSFILTER pfilter)
{
  LPRESOURCEDATAMIN lpres= (LPRESOURCEDATAMIN) pstructReq;

  if (!pfilter->bNullResources)  {
    if (!x_strcmp(lpres->resource_grant_mode,  "N") &&
        !x_strcmp(lpres->resource_convert_mode,"N")
       )
        return  FALSE;     
  }
  return TRUE;
}
static BOOL OthResTypeFilterOK(void * pstructReq,LPSFILTER pfilter)
{
  LPRESOURCEDATAMIN lpres= (LPRESOURCEDATAMIN) pstructReq;

  if (pfilter->ResourceType==RESTYPE_ALL)
    return TRUE;

  if (lpres->resource_type != pfilter->ResourceType)
      return  FALSE;

  return TRUE;
}

static BOOL XResourceFilterOK1(void * pstructReq,LPSFILTER pfilter)
{
  LPRESOURCEDATAMIN lpres= (LPRESOURCEDATAMIN) pstructReq;

  if (!pfilter->bNullResources)  {
    if (!x_strcmp(lpres->resource_grant_mode,  "N") &&
        !x_strcmp(lpres->resource_convert_mode,"N")
       )
        return  FALSE;     
  }
  if (pfilter->ResourceType==RESTYPE_ALL)
    return TRUE;

  if (lpres->resource_type != pfilter->ResourceType)
      return  FALSE;

  return TRUE;
}
static BOOL XResourceFilterOK(void * pstructReq)
{
  return XResourceFilterOK1(pstructReq,&sfiltermem);
}

static BOOL SessionFilterOK1(void * pstructReq,LPSFILTER pfilter)
{
  LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN) pstructReq;
  if (pfilter->bInternalSessions)
     return TRUE;
  if (psessiondta->effective_user[0]==' ' &&
      psessiondta->effective_user[1]=='<'    )
      return FALSE;
  return TRUE;
}
static BOOL SessionFilterOK(void * pstructReq)
{
   return SessionFilterOK1(pstructReq,&sfiltermem);
}

static BOOL TransactionFilterOK1(void * pstructReq,LPSFILTER pfilter)
{
  LPLOGTRANSACTDATAMIN ptransactdta = (LPLOGTRANSACTDATAMIN) pstructReq;
  if (pfilter->bInactiveTransactions)
     return TRUE;
  if (x_stristr((LPUCHAR)(ptransactdta->tx_status), (LPUCHAR)"INACTIVE"))
      return FALSE;
  return TRUE;
}
static BOOL TransactionFilterOK(void * pstructReq)
{
   return TransactionFilterOK1(pstructReq,&sfiltermem);
}

static BOOL LockListFilterOK1(void * pstructReq,LPSFILTER pfilter)
{
  LPLOCKLISTDATAMIN plocklistdta = (LPLOCKLISTDATAMIN) pstructReq;
  if (pfilter->bSystemLockLists)
     return TRUE;
  if (plocklistdta->locklist_related_llb)
     return TRUE;
  return FALSE;
}
static BOOL LockListFilterOK(void * pstructReq)
{
   return LockListFilterOK1(pstructReq,&sfiltermem);
}

static char * TruePath(LPUCHAR bufsrc,char * pdest)
{
  char *pc;
  if (GetPathLoc((char *)bufsrc,&pc) != TRUE)
    pc=(char *)bufsrc;
  x_strcpy(pdest,pc);
  return pdest;
}

static BOOL HaveCommonDiskPlace(LPLOCATIONDATAMIN ploc1,LPLOCATIONDATAMIN ploc2)
{
  int i;
  char buf1[256],buf2[256];
  if (x_stricmp(TruePath(ploc1->LocationArea,buf1),
              TruePath(ploc2->LocationArea,buf2)
             ))
    return FALSE;

  for (i=0;i<5;i++) {
     if (ploc1->LocationUsages[i]==ATTACHED_YES &&
         ploc2->LocationUsages[i]==ATTACHED_YES)
        return TRUE;
  }
  return FALSE;
}

int LocationInLocGroup(int hnode,char * locationname, char * locationgroupname)
{
   LOCATIONDATAMIN loc1;
   LOCATIONDATAMIN loc2;
   int ires;
   if (!x_strcmp(locationname,locationgroupname)) // case where nousage
     return TRUE;                               

   push4mong();
   ires=GetFirstMonInfo(hnode,0,NULL, OT_LOCATION,&loc1,NULL);
   while (ires==RES_SUCCESS) {
      if (!x_strcmp(locationname,(char *)(loc1.LocationName)))
         break;
      ires=GetNextMonInfo(&loc1);
   }
   pop4mong();
   if (ires!=RES_SUCCESS)
     return RES_ERR;
   push4mong();
   ires=GetFirstMonInfo(hnode,0,NULL, OT_LOCATION_NODOUBLE,&loc2,NULL);
   while (ires==RES_SUCCESS) {
      if (!x_strcmp(locationgroupname,(char *)(loc2.LocationName)))
         break;
      ires=GetNextMonInfo(&loc2);
   }
   pop4mong();
   if (ires!=RES_SUCCESS)
     return RES_ERR;
   if (HaveCommonDiskPlace(&loc1,&loc2))
     return RES_SUCCESS;
   return RES_ENDOFDATA;
}

static BOOL ManageLocdoubles(LPLOCATIONDATAMIN ploc)
{
   LOCATIONDATAMIN loc2;
   int ires;
   push4mong();
   ires=GetFirstMonInfo(hnodemem,0,NULL, OT_LOCATION,&loc2,NULL);
   while (ires==RES_SUCCESS) {
      if (!x_strcmp((char *)ploc->LocationName,(char *)(loc2.LocationName))) {
         // first time this area is handled
         while (TRUE) {
            ires=GetNextMonInfo(&loc2);
            if (ires!=RES_SUCCESS)
              break;
            if (HaveCommonDiskPlace(ploc,&loc2))  {
              int iremain=(int)sizeof(ploc->LocationName)-(int)x_strlen((char *)(ploc->LocationName))-1;
              if ((int)x_strlen((char *)(loc2.LocationName)) + 3 >=iremain) {
                 if (iremain<6) 
                    x_strcpy((char *)(ploc->LocationName)+x_strlen((char *)(ploc->LocationName))-5,"..+.");
                 else 
                    x_strcat((char*)(ploc->LocationName),"+..");
              }
              x_strcat((char*)(ploc->LocationName)," + ");
              x_strcat((char*)(ploc->LocationName),(char *)(loc2.LocationName));
            }
         }
         pop4mong();
         return TRUE; // this location (caption eventually handled
      }
      if (HaveCommonDiskPlace(ploc,&loc2))  {
         pop4mong();
         return FALSE;  // Loc Name already included with another one.
      }
      ires=GetNextMonInfo(&loc2);
   }
   pop4mong();
   myerror(ERR_MONITOREQUIV);
   return TRUE;
}

BOOL MonFitWithFilter(int iobjtype,void * pstructReq,LPSFILTER pfilter)
{
  // FINIR FNN: NOUVEAUX TYPES ICE
  switch (iobjtype) {
    case OT_MON_ICE_CONNECTED_USER      :
    case OT_MON_ICE_TRANSACTION         :
    case OT_MON_ICE_CURSOR              :
    case OT_MON_ICE_ACTIVE_USER         :
    case OT_MON_ICE_FILEINFO            :
    case OT_MON_ICE_HTTP_CONNECTION     :
    case OT_MON_ICE_DB_CONNECTION       :
      return TRUE;
  }


  if (!pfilter)
    return TRUE;
  switch (iobjtype) {
    case OT_NODE:
    case OT_NODE_SERVERCLASS:
    case OT_NODE_OPENWIN:
    case OT_NODE_LOGINPSW:
    case OT_NODE_CONNECTIONDATA:
    case OT_NODE_ATTRIBUTE:
    case OT_MON_SERVER:
    case OT_MON_SERVERWITHSESS:
    case OT_MON_ACTIVE_USER:
    case OT_MON_LOCK           :
    case OT_MON_LOCKLIST_LOCK  :
    case OT_MON_RES_LOCK       :
    case OT_MON_LOCKLIST_BL_LOCK:
    case OT_MON_LOGPROCESS:
    case OT_MON_LOGDATABASE:
    case OT_LOCATION:
    case OT_LOCATIONWITHUSAGES:
    case OT_LOCATION_NODOUBLE:
    case OT_MON_REPLIC_SERVER:
    case OT_USER:
    case OT_MON_BLOCKING_SESSION:
    case OT_MON_ICE_CONNECTED_USER:
    case OT_MON_ICE_ACTIVE_USER:
    case OT_MON_ICE_TRANSACTION:
    case OT_MON_ICE_CURSOR:
    case OT_MON_ICE_FILEINFO:
    case OT_MON_ICE_DB_CONNECTION:
      return TRUE;
      break;
    case OT_MON_SESSION:
      return SessionFilterOK1(pstructReq,pfilter);
      break;
    case OT_MON_LOCKLIST:
      return LockListFilterOK1(pstructReq,pfilter);
      break;
    case OTLL_MON_RESOURCE:
      if (!ResFilterOK(pstructReq,pfilter))
        return FALSE;
      return TRUE;
    case OT_MON_RES_OTHER:
      if (!ResFilterOK(pstructReq,pfilter))
        return FALSE;
      if (!OthResTypeFilterOK(pstructReq,pfilter))
        return FALSE;
      return TRUE;
    case OT_MON_TRANSACTION:
      return TransactionFilterOK1(pstructReq,pfilter);
      break;
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_DATABASE:
    case OT_TABLE:
      return TRUE;
      break;
    default:
      return TRUE;
      break;
  }
  return TRUE;
}

static void CompleteSessionBlockState(LPSESSIONDATAMIN psessiondta)
{
	LOCKLISTDATAMIN locklistdta;
	int ires;
	psessiondta->locklist_wait_id=0L;
	push4mong();
	ires=GetFirstMonInfo(hnodemem,OT_MON_SESSION,psessiondta,
						 OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
	while (ires==RES_SUCCESS) {
	   if (locklistdta.locklist_wait_id !=0L){
		  psessiondta->locklist_wait_id = locklistdta.locklist_wait_id;
		  break;
	   }
	   ires=GetNextMonInfo(&locklistdta);
	}
	pop4mong();
}

static BOOL FiltersOK(void * pstructReq)
{
  int  ires,ires1;
  BOOL bRes = FALSE;

  switch (itypemem) {
    case OT_USER:
      {
          LPNODEUSERDATAMIN puserdta = (LPNODEUSERDATAMIN) pstructReq;
          if (!x_strcmp( (char *)(puserdta->User), (char *)(lppublicdispstring()) ))
             return FALSE;
          return TRUE;
      }
    case OT_NODE_OPENWIN:
      {
         WINDOWDATAMIN wnddata = * (LPWINDOWDATAMIN) pstructReq;
         switch (iparenttypemem) {   
            case OT_NODE:
               return TRUE;
            case OT_NODE_SERVERCLASS:
               {
                LPNODESVRCLASSDATAMIN pnodesvrclassdta = (LPNODESVRCLASSDATAMIN) &structparentmem;
                if (!x_strcmp((char *) (wnddata.ServerClass),(char *)(pnodesvrclassdta->ServerClass)))
                    return TRUE;
                 return FALSE;
               }
            case OT_USER:
               {
                 LPNODEUSERDATAMIN puserdta = (LPNODEUSERDATAMIN)&structparentmem;
                 if (!x_strcmp((char *) (wnddata.ConnectUserName),(char *)(puserdta->User)) &&
				     !x_strcmp((char *) (wnddata.ServerClass),    (char *)(puserdta->ServerClass)))
                     return TRUE;
                 return FALSE;
               }
            default:
               myerror(ERR_MONITORTYPE);
               break;
          }
         return FALSE;
         break;
      }

    case OT_MON_SERVERWITHSESS:
    case OT_MON_SERVER:
      {
         if (itypemem==OT_MON_SERVERWITHSESS){
             SERVERDATAMIN svrdata = * (LPSERVERDATAMIN) pstructReq;
             switch (svrdata.servertype) {
               case SVR_TYPE_GCN:
               case SVR_TYPE_NET:
               case SVR_TYPE_RMCMD:
               case SVR_TYPE_JDBC:
               case SVR_TYPE_OTHER:
                  return FALSE;
               case SVR_TYPE_DBMS:
               case SVR_TYPE_STAR:
               case SVR_TYPE_RECOVERY:
               default:
                  break;
             }
         }
         if ( bstructparentmem &&
             (iparenttypemem==OT_DATABASE||iparenttypemem==OT_MON_RES_DB)
           ) {
             SESSIONDATAMIN sessiondta;
             LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN) &structparentmem;
             char DBName[MAXOBJECTNAME];
             char * dbname1 =GetMonDbNameFromKey(pdb->resource_key);
             if (!dbname1) {
               myerror(ERR_PARENTNOEXIST);
               return FALSE;
             }
             x_strcpy(DBName,dbname1);
             push4mong();
             ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,
                                  (void *)pstructReq,
                                  OT_MON_SESSION,&sessiondta,&sfilterALL);
             while (ires==RES_SUCCESS) {
                suppspace((LPUCHAR)sessiondta.db_name);
                if (!x_stricmp(DBName,sessiondta.db_name)){
                   bRes=TRUE;
                   break;
                }
                ires=GetNextMonInfo(&sessiondta);
             }
             pop4mong();
             return bRes;
             break;
         }
         if ( bstructparentmem && iparenttypemem==OT_MON_TRANSACTION) {
             SESSIONDATAMIN sessiondta;
             LPLOGTRANSACTDATAMIN ptransact = (LPLOGTRANSACTDATAMIN) &structparentmem;
             char txsessid[33];
             x_strcpy(txsessid,ptransact->tx_session_id);
             long l_tx_serverpid = ptransact->tx_server_pid;
             push4mong();
             ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,
                                  (void *)pstructReq,
                                  OT_MON_SESSION,&sessiondta,&sfilterALL);
             while (ires==RES_SUCCESS) {
                if (!x_stricmp(txsessid,sessiondta.session_id) && l_tx_serverpid == sessiondta.server_pid ){
                   bRes=TRUE;
                   break;
                }
                ires=GetNextMonInfo(&sessiondta);
             }
             pop4mong();
             return bRes;
             break;
         }
         if ((!bstructparentmem))
            return TRUE;
         myerror(ERR_MONITORTYPE);
         break;
      }
    case OT_MON_ACTIVE_USER:
      {
         SERVERDATAMIN  svrdta;
         SESSIONDATAMIN sessiondta;
         push4mong();
         ires=GetFirstMonInfo(hnodemem,iparenttypemem,NULL,
                              OT_MON_SERVERWITHSESS,&svrdta,NULL);
         while (ires==RES_SUCCESS) {
            push4mong();
            ires1=GetFirstMonInfo(hnodemem,OT_MON_SERVER,&svrdta,
                              OT_MON_SESSION,&sessiondta,&sfilterALL);
            while (ires1==RES_SUCCESS) {
              if (!x_stricmp((char*) pstructReq, sessiondta.effective_user)) {
                 bRes=TRUE;
                 break;
              }
              ires1=GetNextMonInfo(&sessiondta);
            }
            pop4mong();
            if (bRes)
              break;
            ires=GetNextMonInfo(&svrdta);
         }
         pop4mong();
         return bRes;
         break;
      }
    case OT_MON_SESSION:
      {
         return SessionFilterOK(pstructReq);
      }
    case OT_MON_RESOURCE_DTPO:
      {
         LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
         if (!ResFilterOK(pstructReq,&sfiltermem))
           return FALSE;
         switch (presourcedta->resource_type) {
           case RESTYPE_DATABASE:
           case RESTYPE_TABLE:
           case RESTYPE_PAGE:
             return TRUE;
           default:
             return OthResTypeFilterOK(pstructReq,&sfiltermem);
         }
         return TRUE;
      }
    case OTLL_MON_RESOURCE:
      {
         LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
         if (!bstructparentmem) {
            if (!ResFilterOK(pstructReq,&sfiltermem))
               return FALSE;
            return TRUE;
         }
         switch (iparenttypemem) {
           case OT_MON_LOCK: //resource of lock must display even if not in filter
            {
            LPLOCKDATAMIN plock = (LPLOCKDATAMIN) &structparentmem;
            if (plock->resource_id==presourcedta->resource_id)
               return TRUE;
            return FALSE;
            }
            break;
           case OT_DATABASE:
           case OT_MON_RES_DB:
            {
            if (!ResFilterOK(pstructReq,&sfiltermem))
               return FALSE;
            LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN) &structparentmem;
            if (pdb->resource_database_id==presourcedta->resource_database_id)
               return TRUE;
            return FALSE;
            }
            break;
         }
         myerror(ERR_MONITORTYPE);
      }
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_MON_RES_OTHER:
      {
         LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
         RESOURCEDATAMIN curresourcedta = *presourcedta;
         LOCKDATAMIN  lockdta;
         if (!ResFilterOK(pstructReq,&sfiltermem))
            return FALSE;
         if (!ResTypeOK(itypemem,presourcedta->resource_type))
            return FALSE;
         if (itypemem==OT_MON_RES_OTHER) {
            if (!OthResTypeFilterOK(pstructReq,&sfiltermem))
               return FALSE;
         }
         if (!bstructparentmem)
            return TRUE;
         switch (iparenttypemem) {
           case OT_DATABASE:
            {
            LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN) &structparentmem;
            if (pdb->resource_database_id==presourcedta->resource_database_id)
               return TRUE;
            return FALSE;
            }
            break;

           case OT_MON_SERVER:
           case OT_MON_SESSION:
           case OT_MON_TRANSACTION:
            {
             push4mong();
             ires=GetFirstMonInfo(hnodemem,OTLL_MON_RESOURCE,
                                  (void *)&curresourcedta,
                                  OT_MON_LOCK,&lockdta,NULL);
             while (ires==RES_SUCCESS && !bRes) {
                LOCKLISTDATAMIN locklistdta;
                long locklistid = lockdta.locklist_id;
                push4mong();
                ires=GetFirstMonInfo(hnodemem,stack4mong[monstacklevel-2].iparenttypemem,
                                     (void *)&(stack4mong[monstacklevel-2].structparentmem),
                                     OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
                while (ires==RES_SUCCESS) {
                   if (locklistid== locklistdta.locklist_id){
                      bRes=TRUE;
                      break;
                   }
                   ires=GetNextMonInfo(&locklistdta);
                }
                pop4mong();
                ires=GetNextMonInfo(&lockdta);
             }
             pop4mong();
             return bRes;
             break;
            }
         }
         myerror(ERR_MONITORTYPE);
      }
    case OT_MON_LOCKLIST:
      {
        if ( !(iparenttypemem==OT_MON_LOCK && bstructparentmem)) {
           if (!LockListFilterOK(pstructReq))
              return FALSE;
        }

        if (!bstructparentmem)
          return TRUE;
        LPLOCKLISTDATAMIN plocklistdta = (LPLOCKLISTDATAMIN) pstructReq;
        char * lpllsesid = (char *) plocklistdta->locklist_session_id;
		long  ll_serverpid =  plocklistdta->locklist_server_pid;
        if (plocklistdta->bIs4AllLockLists) // special entry for "all locks"
          return FALSE;
        switch (iparenttypemem) {
          case OT_MON_SESSION:
            {
            LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN) &structparentmem;
            if (!x_strcmp(lpllsesid,psessiondta->session_id) && ll_serverpid == psessiondta->server_pid)
               return TRUE;
            return FALSE;
            }
          case OT_MON_SERVER:
          case OT_DATABASE:
          case OT_MON_RES_DB:
            {
            SESSIONDATAMIN sessiondta;

            push4mong();
            ires=GetFirstMonInfo(hnodemem,
                                 stack4mong[monstacklevel-1].iparenttypemem,
                                 (void *)&(stack4mong[monstacklevel-1].structparentmem),
                                 OT_MON_SESSION,&sessiondta,&sfilterALL);
            while (ires==RES_SUCCESS) {
               if (!x_strcmp(lpllsesid,(char *) sessiondta.session_id) && ll_serverpid == sessiondta.server_pid){
                  bRes=TRUE; // no break because of push/popmong issue
               }
               ires=GetNextMonInfo(&sessiondta);
            }
            pop4mong();
            return bRes;
            }
            break;
          case OT_MON_TRANSACTION:
            {
            LPLOGTRANSACTDATAMIN ptransactdta=
                 (LPLOGTRANSACTDATAMIN)&structparentmem;
            if (ptransactdta->tx_transaction_high==plocklistdta->locklist_tx_high &&
                ptransactdta->tx_transaction_low==plocklistdta->locklist_tx_low)
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_LOCK:
            {
            LPLOCKDATAMIN plockdta = (LPLOCKDATAMIN) &structparentmem;
            if (plockdta->locklist_id == plocklistdta->locklist_id)
               return TRUE;
            return FALSE;
            }
            break;
          default:
            break;
        }
        myerror(ERR_MONITORTYPE);
      }
      break;
    case OT_MON_LOCK:
      {
        LOCKLISTDATAMIN locklistdta;
        LPLOCKDATAMIN plockdta = (LPLOCKDATAMIN) pstructReq;
        long locklistid = plockdta->locklist_id;

        if (!bstructparentmem)
          return TRUE;

        switch (iparenttypemem) {
          case OT_DATABASE:
            {
            LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN) &structparentmem;
            if (pdb->resource_database_id==plockdta->resource_database_id)
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_LOCKLIST:
            {
            LPLOCKLISTDATAMIN pll = (LPLOCKLISTDATAMIN) &structparentmem;
            if (pll->locklist_id==plockdta->locklist_id)
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_RES_DB    : // Database Locks on Database
            {
            LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) &structparentmem;
            if (plockdta->locktype==LOCK_TYPE_DB &&
                presourcedta->resource_id==plockdta->resource_id)
               return TRUE;
            return FALSE;
            }
            break;
          case OTLL_MON_RESOURCE:
          case OT_MON_RES_TABLE :
          case OT_MON_RES_PAGE  :
          case OT_MON_RES_OTHER :
            {
            LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) &structparentmem;
            if (presourcedta->resource_id==plockdta->resource_id)
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_SERVER:
            {
            LPSERVERDATAMIN pserverdta = (LPSERVERDATAMIN) &structparentmem;
            SERVERDATAMIN   serverdta  =  *pserverdta;

            push4mong();
            ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,(void *)&serverdta,
                                 OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
            while (ires==RES_SUCCESS) {
               if (locklistid== locklistdta.locklist_id){
                  bRes=TRUE;
                  break;
               }
               ires=GetNextMonInfo(&locklistdta);
            }
            pop4mong();
            return bRes;
            }
            break;
          case OT_MON_SESSION:
            {
            LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN) &structparentmem;
            SESSIONDATAMIN    sessiondta = *psessiondta;

            push4mong();
            ires=GetFirstMonInfo(hnodemem,OT_MON_SESSION,(void *)&sessiondta,
                                 OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
            while (ires==RES_SUCCESS) {
               if (locklistid== locklistdta.locklist_id){
                  bRes=TRUE;
                  break;
               }
               ires=GetNextMonInfo(&locklistdta);
            }
            pop4mong();
            return bRes;
            }
          case OT_MON_TRANSACTION:
            {
            LPLOGTRANSACTDATAMIN ptransactdta = (LPLOGTRANSACTDATAMIN) &structparentmem;
            long lhigh=ptransactdta->tx_transaction_high;
            long llow =ptransactdta->tx_transaction_low;

            push4mong();
            ires=GetFirstMonInfo(hnodemem,0,NULL,
                                 OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
            while (ires==RES_SUCCESS) {
               if (locklistdta.locklist_id      == locklistid) {
                  if ( locklistdta.locklist_tx_high == lhigh &&
                       locklistdta.locklist_tx_low  == llow    ) 
                     bRes=TRUE;
                  break;
               }
               ires=GetNextMonInfo(&locklistdta);
            }
            pop4mong();
            return bRes;
            }
          default:
            break;
        }
        myerror(ERR_MONITORTYPE);
      }
      break;
    case OT_MON_TRANSACTION:
      {
        if (!TransactionFilterOK(pstructReq))
          return FALSE;
        if (!bstructparentmem)
          return TRUE;
        LPLOGTRANSACTDATAMIN ptransact = (LPLOGTRANSACTDATAMIN) pstructReq;
        char * lpllsesid = (char *) ptransact->tx_session_id;
        long l_tx_serverpid = ptransact->tx_server_pid;
         switch (iparenttypemem) {
          case OT_DATABASE:
          case OT_MON_RES_DB:
            {
            LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) &structparentmem;
            char * pc =GetMonDbNameFromKey(presourcedta->resource_key);
            if (!pc)  {
              myerror(ERR_MONITORTYPE);
              return  FALSE;
            }
            suppspace((LPUCHAR)ptransact->tx_db_name);
            if (!x_stricmp(pc,ptransact->tx_db_name))
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_SESSION:
            {
            LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN) &structparentmem;
            if (!x_strcmp(lpllsesid,psessiondta->session_id) && l_tx_serverpid == psessiondta->server_pid )
               return TRUE;
            return FALSE;
            }
            break;
          case OT_MON_SERVER:
            {
            LPSERVERDATAMIN pserverdta = (LPSERVERDATAMIN) &structparentmem;
            SERVERDATAMIN   serverdta = *pserverdta;
            SESSIONDATAMIN sessiondta;

            push4mong();
            ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,(void *)&serverdta,
                                 OT_MON_SESSION,&sessiondta,&sfilterALL);
            while (ires==RES_SUCCESS) {
               if (!x_strcmp(lpllsesid,(char *) sessiondta.session_id) && l_tx_serverpid == sessiondta.server_pid){
                  bRes=TRUE;
                  break;
               }
               ires=GetNextMonInfo(&sessiondta);
            }
            pop4mong();
            return bRes;
            }
            break;
          default:
            break;
        }
        myerror(ERR_MONITORTYPE);
      }
      break;
    case OT_MON_ICE_TRANSACTION:
      {
         if ( bstructparentmem && iparenttypemem==OT_MON_ICE_CONNECTED_USER){
             LPICE_CONNECTED_USERDATAMIN pconnuserdta =(LPICE_CONNECTED_USERDATAMIN) &structparentmem;
             LPICE_TRANSACTIONDATAMIN picetransact = (LPICE_TRANSACTIONDATAMIN) pstructReq;
			 if (x_strcmp((char *)pconnuserdta->name,(char *)picetransact->owner))
				return FALSE;
			 else
				return TRUE;
         }
		 return TRUE;
      }
      break;
    case OT_MON_ICE_CURSOR:
      {
         if ( bstructparentmem && iparenttypemem==OT_MON_ICE_TRANSACTION){
             LPICE_TRANSACTIONDATAMIN ptransactdta =(LPICE_TRANSACTIONDATAMIN) &structparentmem;
             LPICE_CURSORDATAMIN pcursordata = (LPICE_CURSORDATAMIN) pstructReq;
			 if (x_strcmp((char *)ptransactdta->trans_key,(char *)pcursordata->owner))
				return FALSE;
			 else
				return TRUE;
         }
		 return TRUE;
      }
      break;
    case OT_MON_ICE_FILEINFO:
      {
         if ( bstructparentmem && iparenttypemem==OT_MON_ICE_CONNECTED_USER){
             LPICE_CONNECTED_USERDATAMIN pconnuserdta =(LPICE_CONNECTED_USERDATAMIN) &structparentmem;
             LPICE_CACHEINFODATAMIN pcacheinfo = (LPICE_CACHEINFODATAMIN) pstructReq;
			 if (x_strcmp((char *)pconnuserdta->name,(char *)pcacheinfo->owner))
				return FALSE;
			 else
				return TRUE;
         }
		 return TRUE;
      }
      break;
    default:
      break;
  }
  return TRUE;
}
static void FillSesStructFromUser(void *pstructReq,LPUCHAR buf)
{
  LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN)pstructReq;
  memset(psessiondta,'\0',sizeof(SESSIONDATAMIN));
  x_strcpy(psessiondta->effective_user,(char *) buf);
}

void FillResStructFromDB(void *pstructReq,LPUCHAR buf)
{
  LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN)pstructReq;
  memset(presourcedta,'\0',sizeof(RESOURCEDATAMIN));
  presourcedta->resource_type=RESTYPE_DATABASE;
  wsprintf(presourcedta->resource_key,"KEY(DATABASE,%s)",buf);
  x_strcpy(presourcedta->res_database_name,(char *)buf);
}
static void FillLocStruct(void *pstructReq,LPUCHAR buf,LPUCHAR bufowner,LPUCHAR extradata)
{
  LPLOCATIONDATAMIN lploc=(LPLOCATIONDATAMIN) pstructReq;
  fstrncpy(lploc->LocationName,buf,sizeof(lploc->LocationName));
  fstrncpy(lploc->LocationArea,extradata,sizeof(lploc->LocationArea));
  memcpy(lploc->LocationUsages,extradata+MAXOBJECTNAME,
         sizeof(lploc->LocationUsages));
}

static void FillResStructFromTb(int hdl,void * pstructReq,
         LPUCHAR bufparent,LPUCHAR buf,LPUCHAR bufowner,LPUCHAR extradata)
{
  LPRESOURCEDATAMIN lptb = (LPRESOURCEDATAMIN) pstructReq;
  char bufkey[100];
  long DBid;
  wsprintf(bufkey,"KEY(DATABASE,%s)",bufparent);
  if (GetMonDbID(hdl, bufkey, &DBid)!=RES_SUCCESS)
    myerror(ERR_DOMDATA);
  memset(lptb,'\0',sizeof(RESOURCEDATAMIN));
  lptb->hdl = hdl;
  lptb->resource_type = RESTYPE_TABLE;
  lptb->resource_database_id = DBid;
  lptb->resource_table_id =  getint((LPUCHAR)(extradata));
  x_strcpy(lptb->res_database_name,(char *)bufparent);
  return; 
}


#define LOCK_TYPE_ERROR 0
#define LOCK_TYPE_IS    1
#define LOCK_TYPE_IX    2
#define LOCK_TYPE_S     3
#define LOCK_TYPE_SIX   4
#define LOCK_TYPE_X     5

typedef struct type_lock1{
  int ilocktype;
  char *szStringLock;
} STYPELOCK1, FAR *LPSTYPELOCK1;

STYPELOCK1 Type_Lock1 [] = {
	LOCK_TYPE_IS    , "IS",
	LOCK_TYPE_IX    , "IX",
	LOCK_TYPE_S     , "S",
	LOCK_TYPE_SIX   , "SIX",
	LOCK_TYPE_X     , "X",
    -1              , NULL
};

inline static int get_lock_type(char * lpstring)
{
	LPSTYPELOCK1 lptypelock=Type_Lock1;
    while (lptypelock->ilocktype != -1)  {
      if ( !x_strcmp(lptypelock->szStringLock,lpstring))
        return lptypelock->ilocktype;      
	  lptypelock++;
   }
	return LOCK_TYPE_ERROR;
}
inline static BOOL IsLockTypeBlocking(char * lpGrantedLock, int iWaitingLockType)
{
	int iGrantedLockType= get_lock_type(lpGrantedLock);
	if (iGrantedLockType==LOCK_TYPE_ERROR)
		return FALSE;
	BOOL bIsBlocking = TRUE;
	switch (iWaitingLockType) {
		case LOCK_TYPE_IS:
			if (iGrantedLockType!=LOCK_TYPE_X)
				bIsBlocking= FALSE;
			break;
		case LOCK_TYPE_IX:
			if (iGrantedLockType!=LOCK_TYPE_S && iGrantedLockType!=LOCK_TYPE_SIX && iGrantedLockType!=LOCK_TYPE_X)
				bIsBlocking= FALSE;
			break;
		case LOCK_TYPE_S:
			if (iGrantedLockType!=LOCK_TYPE_IX && iGrantedLockType!=LOCK_TYPE_SIX && iGrantedLockType!=LOCK_TYPE_X)
				bIsBlocking= FALSE;
			break;
		case LOCK_TYPE_SIX:
			if (iGrantedLockType==LOCK_TYPE_IS)
				bIsBlocking= FALSE;
			break;
		case LOCK_TYPE_X:
			break;
		default:
			bIsBlocking=FALSE;
			break;
	}
	return bIsBlocking;
}


int GetFirstMonInfo(int       hnodestruct,
                    int       iobjecttypeParent,
                    void     *pstructParent,
                    int       iobjecttypeReq,
                    void     *pstructReq,
                    LPSFILTER pFilter)
{
  int   ires;

  if (iobjecttypeReq==OT_MON_LOCK && iobjecttypeParent==OT_MON_LOCKLIST)
     iobjecttypeReq=OT_MON_LOCKLIST_LOCK;
  if (iobjecttypeReq==OT_LOCATION)
     iobjecttypeReq=OT_LOCATIONWITHUSAGES;
  if (iobjecttypeReq==OT_MON_TRANSACTION && iobjecttypeParent==OT_MON_SESSION)
     pFilter = &sfilterALL;

  hnodemem        =   hnodestruct;
  iparenttypemem  =   iobjecttypeParent;
  itypemem        =   iobjecttypeReq;
  if (pFilter)
     sfiltermem   =   *pFilter;
  else
     memset((char*)&sfiltermem,'\0',sizeof(sfiltermem));
  if (pstructParent)  {
    bstructparentmem = TRUE;
    switch (iparenttypemem) {
      case OT_NODE:
        structparentmem.NodeDta      = * (LPNODEDATAMIN)         pstructParent;break;
      case OT_NODE_SERVERCLASS:
        structparentmem.SvrClassDta  = * (LPNODESVRCLASSDATAMIN) pstructParent;break;
      case OT_USER:
        structparentmem.UsrDta       = * (LPNODEUSERDATAMIN)     pstructParent;break;
      case OT_MON_SERVER:
        structparentmem.SvrDta       = * (LPSERVERDATAMIN)       pstructParent;break;
      case OT_MON_ACTIVE_USER:
      case OT_MON_SESSION:
        structparentmem.SessionDta   = * (LPSESSIONDATAMIN)    pstructParent;break;
      case OT_MON_ICE_CONNECTED_USER:
        structparentmem.IceConnectedUserDta= *(LPICE_CONNECTED_USERDATAMIN)pstructParent;break;
      case OT_MON_ICE_TRANSACTION:
        structparentmem.IceTransactionDta= * (LPICE_TRANSACTIONDATAMIN)pstructParent;break;
      case OT_MON_LOCKLIST:
        structparentmem.LockListDta  = * (LPLOCKLISTDATAMIN)   pstructParent;break;
      case OT_MON_LOCK:
        structparentmem.LockDta      = * (LPLOCKDATAMIN)       pstructParent;break;
      case OT_DATABASE:
      case OTLL_MON_RESOURCE:
      case OT_MON_RES_DB:
      case OT_MON_RES_TABLE:
      case OT_MON_RES_PAGE:
      case OT_MON_RES_OTHER:
        structparentmem.ResourceDta  = * (LPRESOURCEDATAMIN)   pstructParent;break;
      case OT_MON_LOGPROCESS:
        structparentmem.LogProcessDta= * (LPLOGPROCESSDATAMIN) pstructParent;break;
      case OT_MON_LOGDATABASE:
        {
        LPLOGDBDATAMIN  lplogdb = (LPLOGDBDATAMIN) pstructParent;
        FillResStructFromDB(&(structparentmem.ResourceDta),(LPUCHAR)lplogdb->db_name);
        iparenttypemem=OT_DATABASE;
        }
        break;
      case OT_MON_TRANSACTION:
        structparentmem.LogTransacDta= * (LPLOGTRANSACTDATAMIN)pstructParent;break;
      default:
      //  myerror(ERR_MONITORTYPE);
        memset((char*)&structparentmem,'\0',sizeof(structparentmem));
    }
  }
  else {
    bstructparentmem   = FALSE ;
    memset((char*)&structparentmem,'\0',sizeof(structparentmem));
  }

  ASSERT (pstructReq);

  ASSERT( itypemem==OT_NODE         
       || itypemem==OT_NODE_SERVERCLASS
       || itypemem==OT_NODE_OPENWIN
       || itypemem==OT_NODE_LOGINPSW      
       || itypemem==OT_NODE_CONNECTIONDATA
       || itypemem==OT_NODE_ATTRIBUTE
       || itypemem==OT_MON_SERVER
       || itypemem==OT_MON_SERVERWITHSESS 
       || itypemem==OT_MON_LOCKLIST
       || itypemem==OT_MON_LOCK           
       || itypemem==OT_MON_LOCKLIST_LOCK
       || itypemem==OT_MON_LOCKLIST_BL_LOCK
       || itypemem==OT_MON_RES_LOCK 
       || itypemem==OT_MON_SESSION
       || itypemem==OT_MON_LOGPROCESS
       || itypemem==OT_MON_LOGDATABASE
       || itypemem==OT_MON_TRANSACTION
       || itypemem==OT_MON_RES_DB
       || itypemem==OT_MON_RES_TABLE      
       || itypemem==OT_MON_RES_PAGE
       || itypemem==OT_MON_RES_OTHER
       || itypemem==OT_MON_RESOURCE_DTPO
       || itypemem==OT_MON_ACTIVE_USER
       || itypemem==OTLL_MON_RESOURCE
       || itypemem==OT_DATABASE
       || itypemem==OT_TABLE
       || itypemem==OT_USER
       || itypemem==OT_LOCATIONWITHUSAGES
       || itypemem==OT_LOCATION_NODOUBLE
       || itypemem==OT_MON_REPLIC_SERVER
       || itypemem==OT_MON_BLOCKING_SESSION
       || itypemem==OT_MON_ICE_CONNECTED_USER
       || itypemem==OT_MON_ICE_ACTIVE_USER
       || itypemem==OT_MON_ICE_TRANSACTION
       || itypemem==OT_MON_ICE_CURSOR
       || itypemem==OT_MON_ICE_FILEINFO
       || itypemem==OT_MON_ICE_DB_CONNECTION

  );
  switch (itypemem) {
    case OT_MON_LOCK:
      {
      LOCKLISTDATAMIN locklstdta;
      if (bstructparentmem && iparenttypemem==OT_MON_RES_DB) {
         int ires=DB2Resource(hnodestruct,&(structparentmem.ResourceDta));
         if (ires!=RES_SUCCESS)
            return ires; //if not a locked resource, should be RES_ENDOFDATA
      }
      if (bstructparentmem && iparenttypemem==OT_DATABASE) {
         if (FillDBId(hnodestruct,&(structparentmem.ResourceDta))
              !=RES_SUCCESS)
            return RES_ERR;
      }
      locklstdta.bIs4AllLockLists = TRUE;
      ires=DOMGetFirstObject(hnodestruct, OT_MON_LOCKLIST_LOCK, 0,
                               (LPUCHAR *) &locklstdta,
                               TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      }
      break;
    case OTLL_MON_RESOURCE:
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_MON_RES_OTHER:
    case OT_MON_RESOURCE_DTPO:
      {
      if (bstructparentmem && iparenttypemem==OT_DATABASE) {
         if (FillDBId(hnodestruct,&(structparentmem.ResourceDta))
              !=RES_SUCCESS)
            return RES_ERR;
      }
      ires=DOMGetFirstObject(hnodestruct, OTLL_MON_RESOURCE, 0, NULL,
                               TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      }
      break;
    case OT_MON_ACTIVE_USER:
      {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      ires=DOMGetFirstObject(hnodestruct, OT_USER, 0, NULL,
                            TRUE, NULL, buf,bufowner,extradata);
      if (ires==RES_SUCCESS && !FiltersOK(buf))
         return GetNextMonInfo(pstructReq);
      if (ires==RES_SUCCESS)
         FillSesStructFromUser(pstructReq,buf);
      return ires;
      }
    case OT_DATABASE:
      {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      ires=DOMGetFirstObject(hnodestruct, OT_DATABASE, 0, NULL,
                            TRUE, NULL, buf,bufowner,extradata);
      if (ires==RES_SUCCESS) {
         FillResStructFromDB(pstructReq,buf);
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN)pstructReq;
		 presourcedta->dbtype = getint(extradata+STEPSMALLOBJ);
	  }
      return ires;
      }
    case OT_LOCATION_NODOUBLE:
    case OT_LOCATIONWITHUSAGES:
      {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      ires=DOMGetFirstObject(hnodestruct, OT_LOCATIONWITHUSAGES, 0, NULL,
                            TRUE, NULL, buf,bufowner,extradata);
      if (itypemem==OT_LOCATION_NODOUBLE) {
         while (ires==RES_SUCCESS) {
           FillLocStruct(pstructReq,buf,bufowner,extradata);
           BOOL bKeep=ManageLocdoubles((LPLOCATIONDATAMIN)pstructReq);
           if (bKeep)
              return ires;
           ires=DOMGetNextObject(buf,bufowner,extradata);
         }
         return ires;
      }
      if (ires==RES_SUCCESS)
         FillLocStruct(pstructReq,buf,bufowner,extradata);
      return ires;
      }
    case OT_USER:
      {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      LPNODEUSERDATAMIN lpuser=(LPNODEUSERDATAMIN) pstructReq;
      switch (iparenttypemem) {
         case OT_NODE:
           {
               LPNODEDATAMIN lpnode= (LPNODEDATAMIN) &(structparentmem.NodeDta);
               lstrcpy((char *) buf,(char *)lpnode->NodeName);
               lstrcpy((char *)lpuser->NodeName,(char *)lpnode->NodeName);
               lstrcpy((char *)lpuser->ServerClass,"");     // no server class in this case
               lpuser->bIsLocal          =lpnode->bIsLocal;
               lpuser->isSimplifiedModeOK=lpnode->isSimplifiedModeOK;
			   int  oldOIVers = GetOIVers();
			   BOOL bConnectOK = CheckConnection(buf,FALSE,TRUE);
			   int  remoteoivers=GetOIVers();
	           SetOIVers(oldOIVers);
			   if (!bConnectOK)
				   return RES_ERR;
			   if (remoteoivers==OIVERS_NOTOI)
				   return RES_NOT_OI;
               break;
           }
         case OT_NODE_SERVERCLASS:
            {
               LPNODESVRCLASSDATAMIN lpserverclass = (LPNODESVRCLASSDATAMIN) &structparentmem;
               if (x_stricmp((char *)lpserverclass->ServerClass,"INGRES"))
                  return RES_NOT_OI;
               wsprintf((char *) buf,
                        "%s%s%s%s",
                        (char *)lpserverclass->NodeName,
                        LPCLASSPREFIXINNODENAME,
                        (char *)lpserverclass->ServerClass,
                        LPCLASSSUFFIXINNODENAME
                        );
               lstrcpy((char *)lpuser->NodeName,   (char *)lpserverclass->NodeName);
               lstrcpy((char *)lpuser->ServerClass,(char *)lpserverclass->ServerClass);  
               lpuser->bIsLocal          =lpserverclass->bIsLocal;
               lpuser->isSimplifiedModeOK=lpserverclass->isSimplifiedModeOK;
               break;
           }
      }
      hnodestruct=hnodemem=OpenNodeStruct(buf);

      if (hnodestruct<0) {
        //_T("Maximum number of connections has been reached - Users list cannot be refreshed in Node Window")
        CString csMsg = VDBA_MfcResourceString(IDS_MAX_NB_CONNECT);
        csMsg += VDBA_MfcResourceString(IDS_E_USERS_LIST);
        MessageBox(GetFocus(),
                   csMsg,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION);
         return RES_ERR;
      }
      ires=DOMGetFirstObject(hnodestruct, OT_USER, 0, NULL,
                            TRUE, NULL, (LPUCHAR)(&lpuser->User),bufowner,extradata);
      if (ires==RES_SUCCESS && !FiltersOK(pstructReq))
         return GetNextMonInfo(pstructReq);
      if (ires!=RES_SUCCESS)
         CloseNodeStruct(hnodestruct,TRUE);
      return ires;
      }
    case OT_TABLE:
      {
      LPRESOURCEDATAMIN lpparent = (LPRESOURCEDATAMIN) &structparentmem;
      if (!lpparent)
         return RES_ERR;
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      UCHAR bufparent[MAXOBJECTNAME];
      LPUCHAR parents[3];
      x_strcpy((char *)bufparent,GetMonDbNameFromKey(lpparent->resource_key));
      parents[0]=bufparent;
      ires=DOMGetFirstObject(hnodestruct, OT_TABLE, 1, parents,
                            TRUE, NULL, buf,bufowner,extradata);
      if (ires==RES_SUCCESS)
         FillResStructFromTb(hnodemem,pstructReq,bufparent,buf,bufowner,extradata);
      return ires;
      }
  	case OT_MON_BLOCKING_SESSION:
	{
        long blockid;
	    if (iparenttypemem==OT_MON_LOCKLIST) {
          LPLOCKLISTDATAMIN plocklist = (LPLOCKLISTDATAMIN)&structparentmem;
          blockid = plocklist->locklist_wait_id;
		}
		else {
			if (iparenttypemem==OT_MON_SESSION) {
			  LPSESSIONDATAMIN psessiondta = (LPSESSIONDATAMIN) &structparentmem;
			  blockid = psessiondta->locklist_wait_id;
			}
			else {
		    	  myerror(ERR_MONITORTYPE);
			      return RES_ERR;
			}
		}
		LOCKDATAMIN lockdta,cur_lockdta;
		if (!blockid)
          return RES_ENDOFDATA; // no blocked lock
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,OT_MON_LOCK,&lockdta,NULL);
        while (ires==RES_SUCCESS) {
          if (blockid==lockdta.lock_id) 
            break;
          ires=GetNextMonInfo(&lockdta);
        }
        pop4mong();
		if (ires!=RES_SUCCESS)
			return ires;
		if (x_stricmp(lockdta.lock_state,"WAITING"))
			return RES_ERR;

        structparentmem.LockDta      = lockdta;

		// blocked lock is found, search for other locks on same resource
		long resource_id=lockdta.resource_id;
		int iBlockedLockType= get_lock_type(lockdta.lock_request_mode);
		if (iBlockedLockType==LOCK_TYPE_ERROR)
			return RES_ERR;
		push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,OT_MON_LOCK,&cur_lockdta,NULL);
		while (ires==RES_SUCCESS) {
          if (cur_lockdta.resource_id==resource_id) {
			  if (x_stricmp(cur_lockdta.lock_state,"WAITING"))  {
				if (x_stricmp(cur_lockdta.lock_state,"GRANTED")) {
					ires=RES_CANNOT_SOLVE;
					break;
				}
				if (IsLockTypeBlocking(cur_lockdta.lock_request_mode, iBlockedLockType)) {
					// blocking lock found. get the session
					push4mong();
					ires= GetFirstMonInfo(hnodestruct,OT_MON_LOCK,&cur_lockdta,OT_MON_SESSION,pstructReq,&sfilterALL);
					pop4mong();
					if (ires==RES_SUCCESS) {
						itypemem=OT_MON_BLOCKING_SESSION;
						iparenttypemem=OT_MON_LOCK;
						return ires;
					}
					myerror(ERR_MONITOREQUIV);
				}
			  }
          }
		  ires=GetNextMonInfo(&cur_lockdta);
        }
        pop4mong();
		return ires;  // none found, or error
	}
    break;
    
    case OT_MON_SESSION:
      if (iparenttypemem==OT_MON_SERVER)  {
        ires=DOMGetFirstObject(hnodestruct, itypemem, 0, (LPUCHAR *)&structparentmem,
                              TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
		if (ires==RES_SUCCESS)
			CompleteSessionBlockState((LPSESSIONDATAMIN)pstructReq);
        break;
      }
      if (iparenttypemem==OT_MON_LOCK) {
        LPLOCKDATAMIN plock = (LPLOCKDATAMIN)&structparentmem;
        LOCKLISTDATAMIN locklistdta;
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        long locklistid = plock->locklist_id;
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
        while (ires==RES_SUCCESS) {
          if (locklistid==locklistdta.locklist_id)
            break;
          ires=GetNextMonInfo(&locklistdta);
        }
        pop4mong();
        if (ires!=RES_SUCCESS)
          return ires;
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,NULL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,&sfilterALL);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(sessiondta.session_id,locklistdta.locklist_session_id) && sessiondta.server_pid == locklistdta.locklist_server_pid ) {
               * (LPSESSIONDATAMIN)pstructReq = sessiondta;
               pop4mong();
               pop4mong();
               return ires;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();

        return ires; // none found
      }
      if (iparenttypemem==OT_DATABASE || iparenttypemem==OT_MON_RES_DB) {
        LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN)&structparentmem;
        char localdbname[MAXOBJECTNAME];
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        char * pc = GetMonDbNameFromKey(pdb->resource_key);
        if (!pc)
            return RES_ERR;
        x_strcpy(localdbname,pc);
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,NULL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,pFilter);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(localdbname,sessiondta.db_name)) {
               iparenttypemem=OT_DATABASE;
               * (LPSESSIONDATAMIN)pstructReq = sessiondta;
               return RES_SUCCESS;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();
        return ires; // none found
      }

      if (iparenttypemem==OT_MON_ACTIVE_USER) {
        LPSESSIONDATAMIN puser = (LPSESSIONDATAMIN)&structparentmem;
        char localusername[MAXOBJECTNAME];
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        x_strcpy(localusername,puser->effective_user);
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,NULL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,pFilter);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(localusername,sessiondta.effective_user)) {
               iparenttypemem=OT_MON_ACTIVE_USER;
               * (LPSESSIONDATAMIN)pstructReq = sessiondta;
               return RES_SUCCESS;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();
        return ires; // none found
      }
      if (iparenttypemem==OT_MON_TRANSACTION) {
        LPLOGTRANSACTDATAMIN ptransact = (LPLOGTRANSACTDATAMIN)&structparentmem;
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        char txsessid[33];
        long l_tx_serverpid = ptransact->tx_server_pid;
       x_strcpy(txsessid,ptransact->tx_session_id);
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,NULL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,&sfilterALL);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(sessiondta.session_id,txsessid) && l_tx_serverpid == sessiondta.server_pid) {
               * (LPSESSIONDATAMIN)pstructReq = sessiondta;
               pop4mong();
               pop4mong();
               return ires;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();
        return ires; // none found
      }
      if (iparenttypemem==OT_MON_LOCKLIST) {
        LPLOCKLISTDATAMIN plocklist = (LPLOCKLISTDATAMIN)&structparentmem;
        SERVERDATAMIN  svrdta;
        SESSIONDATAMIN sessiondta;
        char sessid[33];
        x_strcpy(sessid,plocklist->locklist_session_id);
  		long  ll_serverpid =  plocklist->locklist_server_pid;
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,&sfilterALL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,&sfilterALL);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(sessiondta.session_id,sessid) && sessiondta.server_pid == ll_serverpid  ) {
               * (LPSESSIONDATAMIN)pstructReq = sessiondta;
               pop4mong();
               pop4mong();
               return ires;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();
        return ires; // none found
      }
      break;
    case OT_MON_LOCKLIST_BL_LOCK:
      if (iparenttypemem==OT_MON_LOCKLIST) {
        LPLOCKLISTDATAMIN plocklist = (LPLOCKLISTDATAMIN)&structparentmem;
        LOCKDATAMIN lockdta;
        long blockid = plocklist->locklist_wait_id;
        if (!blockid)
          return RES_ENDOFDATA; // no blocked lock
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,OT_MON_LOCK,&lockdta,&sfilterALL);
        while (ires==RES_SUCCESS) {
          if (blockid==lockdta.lock_id) {
            * (LPLOCKDATAMIN)pstructReq = lockdta;
            break;
          }
          ires=GetNextMonInfo(&lockdta);
        }
        pop4mong();
        return ires;
      }
      break;
    case OT_MON_SERVER:
    case OT_MON_SERVERWITHSESS:
      if (!bstructparentmem || iparenttypemem==OT_DATABASE
                            || iparenttypemem== OT_MON_RES_DB
                            || iparenttypemem== OT_MON_TRANSACTION
                            )  {
        ires=DOMGetFirstObject(hnodestruct, OT_MON_SERVER, 0, NULL,
                               TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
        break;
      }
      if (iparenttypemem==OT_MON_LOCK) {
        LPLOCKDATAMIN plock = (LPLOCKDATAMIN)&structparentmem;
        LOCKLISTDATAMIN locklistdta;
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        long locklistid = plock->locklist_id;
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_LOCKLIST,&locklistdta,&sfilterALL);
        while (ires==RES_SUCCESS) {
          if (locklistid==locklistdta.locklist_id)
            break;
          ires=GetNextMonInfo(&locklistdta);
        }
        pop4mong();
        if (ires!=RES_SUCCESS)
          return ires;
        push4mong();
        ires=GetFirstMonInfo(hnodestruct,0,NULL,
                   OT_MON_SERVERWITHSESS,&svrdta,&sfilterALL);
        while (ires==RES_SUCCESS) {
          push4mong();
          ires=GetFirstMonInfo(hnodestruct,OT_MON_SERVER,&svrdta,
                   OT_MON_SESSION,&sessiondta,&sfilterALL);
          while (ires==RES_SUCCESS) {
            if (!x_stricmp(sessiondta.session_id,locklistdta.locklist_session_id) && sessiondta.server_pid == locklistdta.locklist_server_pid ) {
               * (LPSERVERDATAMIN)pstructReq = svrdta;
               pop4mong();
               pop4mong();
               return ires;
            }
            ires=GetNextMonInfo(&sessiondta);
          }
          pop4mong();
          ires=GetNextMonInfo(&svrdta);
        }
        pop4mong();
        return ires; // not found
      }
      break;
	case OT_MON_ICE_TRANSACTION:
	{
	  LPUCHAR * pp1;
      if (iparenttypemem==OT_MON_ICE_CONNECTED_USER)  {
		LPSERVERDATAMIN psvr = & structparentmem.IceConnectedUserDta.svrdata;
		pp1=(LPUCHAR *) psvr;
      }
	  else
		  pp1=(LPUCHAR *) &structparentmem;
      ires=DOMGetFirstObject(hnodestruct, itypemem, 0, pp1,
                              TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      break;
	}
	case OT_MON_ICE_CURSOR:
	{
	  LPUCHAR * pp1;
      if (iparenttypemem==OT_MON_ICE_TRANSACTION)  {
		LPSERVERDATAMIN psvr = & structparentmem.IceTransactionDta.svrdata;
		pp1=(LPUCHAR *) psvr;
      }
	  else
		  pp1=(LPUCHAR *) &structparentmem;
      ires=DOMGetFirstObject(hnodestruct, itypemem, 0, pp1,
                              TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      break;
	}
	case OT_MON_ICE_FILEINFO:
	{
	  LPUCHAR * pp1;
      if (iparenttypemem==OT_MON_ICE_CONNECTED_USER)  {
		LPSERVERDATAMIN psvr = & structparentmem.IceConnectedUserDta.svrdata;
		pp1=(LPUCHAR *) psvr;
      }
	  else
		  pp1=(LPUCHAR *) &structparentmem;
      ires=DOMGetFirstObject(hnodestruct, itypemem, 0, pp1,
                              TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      break;
	}
	case OT_NODE_OPENWIN:
	{
		if (iparenttypemem==OT_NODE_SERVERCLASS)  { // filtering done in FiltersOK, so take all windows on node for now
			LPNODESVRCLASSDATAMIN lpnodesvrclass = (LPNODESVRCLASSDATAMIN) pstructParent;
			NODEDATAMIN NodeDta;
			x_strcpy((char *) (NodeDta.NodeName), (char *)lpnodesvrclass->NodeName);
			NodeDta.bIsLocal           = lpnodesvrclass->bIsLocal;
			NodeDta.isSimplifiedModeOK = lpnodesvrclass->isSimplifiedModeOK;
			NodeDta.bWrongLocalName    = FALSE;
			NodeDta.bInstallPassword   = FALSE;
			ires=DOMGetFirstObject(hnodestruct, itypemem, 0, (LPUCHAR *) &NodeDta,
						TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
		}
		else {
			if (iparenttypemem==OT_USER)  { // filtering done in FiltersOK, so take all windows on node for now
				LPNODEUSERDATAMIN puserdta = (LPNODEUSERDATAMIN)pstructParent;
				NODEDATAMIN NodeDta;
				x_strcpy((char *) (NodeDta.NodeName), (char *)puserdta->NodeName);
				NodeDta.bIsLocal           = puserdta->bIsLocal;
				NodeDta.isSimplifiedModeOK = puserdta->isSimplifiedModeOK;
				NodeDta.bWrongLocalName    = FALSE;
				NodeDta.bInstallPassword   = FALSE;
				ires=DOMGetFirstObject(hnodestruct, itypemem, 0, (LPUCHAR *) &NodeDta,
							TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
			}
			else {
				LPUCHAR * lplppar=NULL;
				if (bstructparentmem)
					lplppar=(LPUCHAR *) &structparentmem;
				ires=DOMGetFirstObject(hnodestruct, itypemem, 0, lplppar,
					TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
			}
		}
		break;
	}

    default:
      LPUCHAR * lplppar=NULL;
      if (bstructparentmem)
        lplppar=(LPUCHAR *) &structparentmem;
      ires=DOMGetFirstObject(hnodestruct, itypemem, 0, lplppar,
                              TRUE, NULL, (LPUCHAR)pstructReq, NULL, NULL);
      break;
  }
  if (ires==RES_SUCCESS && !FiltersOK(pstructReq))
    return GetNextMonInfo(pstructReq);

  if (ires==RES_SUCCESS) {
	  if (itypemem == OT_MON_RES_DB) {  // set the dbtype for the database icon
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
		 presourcedta->dbtype=GetDBTypeFromKey(hnodestruct, presourcedta->resource_key);
	  }
	  if (itypemem == OTLL_MON_RESOURCE) {  // set the dbtype for the database icon
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
		 if (presourcedta->resource_type == 1)
			 presourcedta->dbtype=GetDBTypeFromKey(hnodestruct, presourcedta->resource_key);
	  }
		if (itypemem == OT_MON_LOGDATABASE) {  // set the dbtype for the database icon
		 LPLOGDBDATAMIN plogdta = (LPLOGDBDATAMIN) pstructReq;
		 plogdta->dbtype=GetDBTypeFromName(hnodestruct, plogdta->db_name);
	  }
  }
 
  return ires;
}

int GetNextMonInfo(void *pstructReq)
{
  int ires;

  if (itypemem==OT_MON_ACTIVE_USER) {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      while (TRUE) {
        ires=DOMGetNextObject  (buf,bufowner,extradata);
        if (ires!=RES_SUCCESS)
           break;
        if (FiltersOK(buf)) {
           FillSesStructFromUser(pstructReq,buf);
           break;
        }
      }
      return ires;
  }
  if (itypemem==OT_DATABASE) {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[MAXOBJECTNAME];
      ires=DOMGetNextObject(buf,bufowner,extradata);
      if (ires==RES_SUCCESS) {
         FillResStructFromDB(pstructReq,buf);
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN)pstructReq;
		 presourcedta->dbtype = getint(extradata+STEPSMALLOBJ);
	  }
      return ires;
  }
  if (itypemem==OT_LOCATIONWITHUSAGES || itypemem==OT_LOCATION_NODOUBLE) {
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      ires=DOMGetNextObject(buf,bufowner,extradata);
      if (itypemem==OT_LOCATION_NODOUBLE) {
         while (ires==RES_SUCCESS) {
           FillLocStruct(pstructReq,buf,bufowner,extradata);
           BOOL bKeep=ManageLocdoubles((LPLOCATIONDATAMIN)pstructReq);
           if (bKeep)
              return ires;
           ires=DOMGetNextObject(buf,bufowner,extradata);
         }
         return ires;
      }
      if (ires==RES_SUCCESS)
         FillLocStruct(pstructReq,buf,bufowner,extradata);
      return ires;
  }
  if (itypemem==OT_TABLE) {
      LPRESOURCEDATAMIN lpparent = (LPRESOURCEDATAMIN)&structparentmem;
      UCHAR buf[MAXOBJECTNAME];
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      UCHAR bufparent[MAXOBJECTNAME];
      x_strcpy((char *)bufparent,GetMonDbNameFromKey(lpparent->resource_key));
      ires=DOMGetNextObject(buf,bufowner,extradata);
      if (ires==RES_SUCCESS)
         FillResStructFromTb(hnodemem,pstructReq,bufparent,buf,bufowner,extradata);
      return ires;
  }

  if (itypemem==OT_USER) {
      UCHAR bufowner[MAXOBJECTNAME];
      UCHAR extradata[EXTRADATASIZE];
      LPNODEUSERDATAMIN lpuser=(LPNODEUSERDATAMIN) pstructReq;
      switch (iparenttypemem) {
         case OT_NODE:
           {
               LPNODEDATAMIN lpnode= (LPNODEDATAMIN) &(structparentmem.NodeDta);
               lstrcpy((char *)lpuser->NodeName,(char *)lpnode->NodeName);
               lstrcpy((char *)lpuser->ServerClass,"");     // no server class in this case
               lpuser->bIsLocal          =lpnode->bIsLocal;
               lpuser->isSimplifiedModeOK=lpnode->isSimplifiedModeOK;
               break;
           }
         case OT_NODE_SERVERCLASS:
            {
               LPNODESVRCLASSDATAMIN lpserverclass = (LPNODESVRCLASSDATAMIN) &structparentmem;
               if (x_stricmp((char *)lpserverclass->ServerClass,"INGRES"))
                  return RES_ENDOFDATA;
               lstrcpy((char *)lpuser->NodeName,   (char *)lpserverclass->NodeName);
               lstrcpy((char *)lpuser->ServerClass,(char *)lpserverclass->ServerClass);  
               lpuser->bIsLocal          =lpserverclass->bIsLocal;
               lpuser->isSimplifiedModeOK=lpserverclass->isSimplifiedModeOK;
               break;
           }
      }
      while (TRUE) {
         ires=DOMGetNextObject((LPUCHAR)(&lpuser->User),bufowner,extradata);
         if (ires!=RES_SUCCESS) {
            CloseNodeStruct(hnodemem,TRUE);
            break;
         }
         if ( FiltersOK(pstructReq))
            break;
      }
      return ires;
  }
  if (itypemem==OT_MON_BLOCKING_SESSION){
        iparenttypemem=0;
		itypemem= OT_MON_LOCK;
        LPLOCKDATAMIN pblockedlock = (LPLOCKDATAMIN)&(stack4mong[monstacklevel-1].structparentmem);
        LOCKDATAMIN cur_lockdta;
		long resource_id=pblockedlock->resource_id;
		int iBlockedLockType= get_lock_type(pblockedlock->lock_request_mode);
		ires=GetNextMonInfo(&cur_lockdta);
        while(ires==RES_SUCCESS) {
          if (cur_lockdta.resource_id==resource_id) {
			 if (x_stricmp(cur_lockdta.lock_state,"WAITING"))  {
    			if (x_stricmp(cur_lockdta.lock_state,"GRANTED")) {
					ires=RES_CANNOT_SOLVE;
					break;
				}
				if (IsLockTypeBlocking(cur_lockdta.lock_request_mode, iBlockedLockType)) {
					// blocking lock found. get the session
					push4mong();
					ires= GetFirstMonInfo(hnodemem,OT_MON_LOCK,&cur_lockdta,OT_MON_SESSION,pstructReq,&sfilterALL);
					pop4mong();
					if (ires==RES_SUCCESS) {
						itypemem=OT_MON_BLOCKING_SESSION;
						iparenttypemem=OT_MON_LOCK;
						return ires;
					}
					myerror(ERR_MONITOREQUIV);
				}
			 }
		  }
		  ires=GetNextMonInfo(&cur_lockdta);
		}
        pop4mong();
		return ires;  // end of data, or error
  }
                
  if (itypemem==OT_MON_SESSION){
      if (iparenttypemem==OT_MON_SERVER && bstructparentmem) {
		  while (TRUE) {
			ires=DOMGetNextObject  ((LPUCHAR)pstructReq, NULL, NULL);
			if (ires!=RES_SUCCESS)
			   break;
			if (FiltersOK(pstructReq)) {
				CompleteSessionBlockState((LPSESSIONDATAMIN)pstructReq);
                break;
			}
		  }
		  return ires;

	  }
      if (iparenttypemem==OT_MON_LOCK && bstructparentmem)
        return RES_ENDOFDATA;
      if (iparenttypemem==OT_MON_TRANSACTION && bstructparentmem)
        return RES_ENDOFDATA;
      if (iparenttypemem==OT_MON_LOCKLIST  && bstructparentmem)
        return RES_ENDOFDATA;

      if (iparenttypemem==OT_MON_ACTIVE_USER && bstructparentmem) {
        iparenttypemem=OT_MON_SERVER;
        LPSESSIONDATAMIN puser = (LPSESSIONDATAMIN)&(stack4mong[monstacklevel-2].structparentmem);
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;

        ires=GetNextMonInfo(&sessiondta);
        while(TRUE) {
          switch (ires) {
            case RES_SUCCESS:
               if (!x_stricmp(puser->effective_user,sessiondta.effective_user)) {
                  iparenttypemem=OT_MON_ACTIVE_USER;
                  * (LPSESSIONDATAMIN)pstructReq = sessiondta;
                  return RES_SUCCESS;
               }
               ires=GetNextMonInfo(&sessiondta);
               break;
            case RES_ENDOFDATA:
               pop4mong();
               ires=GetNextMonInfo(&svrdta);
               if (ires!=RES_SUCCESS) {
                  pop4mong();
                  return ires;
               }
               push4mong();
               ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,&svrdta,
                        OT_MON_SESSION,&sessiondta,&(stack4mong[monstacklevel-2].sfiltermem));
               break;
            default:
               pop4mong();
               pop4mong();
               return RES_ERR;
          }
        }
      }
      if (iparenttypemem==OT_DATABASE && bstructparentmem) {
        iparenttypemem=OT_MON_SERVER;
        LPRESOURCEDATAMIN pdb = (LPRESOURCEDATAMIN)&(stack4mong[monstacklevel-2].structparentmem);
        SERVERDATAMIN svrdta;
        SESSIONDATAMIN sessiondta;
        char * pdbname = GetMonDbNameFromKey(pdb->resource_key);
        ires=GetNextMonInfo(&sessiondta);
        while(TRUE) {
          switch (ires) {
            case RES_SUCCESS:
               if (!x_stricmp(pdbname,sessiondta.db_name)) {
                  iparenttypemem=OT_DATABASE;
                  * (LPSESSIONDATAMIN)pstructReq = sessiondta;
                  return RES_SUCCESS;
               }
               ires=GetNextMonInfo(&sessiondta);
               break;
            case RES_ENDOFDATA:
               pop4mong();
               ires=GetNextMonInfo(&svrdta);
               if (ires!=RES_SUCCESS) {
                  pop4mong();
                  return ires;
               }
               push4mong();
               ires=GetFirstMonInfo(hnodemem,OT_MON_SERVER,&svrdta,
                        OT_MON_SESSION,&sessiondta,&(stack4mong[monstacklevel-2].sfiltermem));
               break;
            default:
               pop4mong();
               pop4mong();
               return RES_ERR;
          }
        }
      }
  }
  if (itypemem==OT_MON_SERVER || itypemem==OT_MON_SERVERWITHSESS) {
      if (iparenttypemem==OT_MON_LOCK && bstructparentmem)
        return RES_ENDOFDATA;
  }
  if (itypemem==OT_MON_LOCKLIST_BL_LOCK) {
        return RES_ENDOFDATA;
  }
  while (TRUE) {
    ires=DOMGetNextObject  ((LPUCHAR)pstructReq, NULL, NULL);
    if (ires!=RES_SUCCESS)
       break;
    if (FiltersOK(pstructReq)) {
   	  if (itypemem == OT_MON_RES_DB) {  // set the dbtype for the database icon
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
		 presourcedta->dbtype=GetDBTypeFromKey(hnodemem, presourcedta->resource_key);
	  }
	  if (itypemem == OTLL_MON_RESOURCE) {  // set the dbtype for the database icon
		 LPRESOURCEDATAMIN presourcedta = (LPRESOURCEDATAMIN) pstructReq;
		 if (presourcedta->resource_type == 1)
			 presourcedta->dbtype=GetDBTypeFromKey(hnodemem, presourcedta->resource_key);
	  }
	  if (itypemem == OT_MON_LOGDATABASE) {  // set the dbtype for the database icon
		 LPLOGDBDATAMIN plogdta = (LPLOGDBDATAMIN) pstructReq;
		 plogdta->dbtype=GetDBTypeFromName(hnodemem, plogdta->db_name);
	  }
	  break;
	}
  }
  return ires;
}

int MonGetRelatedInfo(int       hnodestruct,
                      int       iobjecttypeSrc,
                      void     *pstructSrc,
                      int       iobjecttypeDest,
                      void     *pstructDest)
{
   switch (iobjecttypeSrc) {
      case OT_MON_SESSION:
         switch (iobjecttypeDest) {
            case OT_MON_SERVER:
               {
                 LPSERVERDATAMIN  lpdest    = (LPSERVERDATAMIN) pstructDest;
                 LPSESSIONDATAMIN lpsession = (LPSESSIONDATAMIN) pstructSrc;
                 *lpdest = lpsession->sesssvrdata;
                 return RES_SUCCESS;
               }
               break;
         }
         break;
      default:
         break;
   }
   myerror(ERR_MONITORTYPE);
   return RES_ERR;
}

static int cmplong(long l1, long l2)
{
  if (l1>l2)
     return 1;
  if (l1<l2)
     return -1;
  return 0;
}

int CompareMonInfo(int iobjecttype, void *pstruct1, void *pstruct2)
{
   return CompareMonInfo1(iobjecttype, pstruct1, pstruct2, FALSE);
}

int CompareMonInfo1(int iobjecttype, void *pstruct1, void *pstruct2, BOOL bUpdSameObj)
{
  int ires;
  ASSERT(pstruct1);
  ASSERT(pstruct2);


  switch (iobjecttype) {
    case OT_NODE:
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
        return x_stricmp((char *)lpn1->NodeName,(char *)lpn2->NodeName);
      }
      break;
    case OT_NODE_SERVERCLASS:
      {
        LPNODESVRCLASSDATAMIN lpn1 = (LPNODESVRCLASSDATAMIN) pstruct1;
        LPNODESVRCLASSDATAMIN lpn2 = (LPNODESVRCLASSDATAMIN) pstruct2;
        return x_stricmp((char *)lpn1->ServerClass,(char *)lpn2->ServerClass);
      }
      break;
    case OT_USER:
      {
        LPNODEUSERDATAMIN lpn1 = (LPNODEUSERDATAMIN) pstruct1;
        LPNODEUSERDATAMIN lpn2 = (LPNODEUSERDATAMIN) pstruct2;
        return x_stricmp((char *)lpn1->User,(char *)lpn2->User);
      }
      break;
    case OT_NODE_OPENWIN:
      {
        LPWINDOWDATAMIN lpn1 = (LPWINDOWDATAMIN) pstruct1;
        LPWINDOWDATAMIN lpn2 = (LPWINDOWDATAMIN) pstruct2;
        // Emb 10/04 : Needs complimentary test on windows handle for load environment
        ires = x_stricmp((char *)lpn1->Title,(char *)lpn2->Title);
        if (ires)
          return ires;
        else {
          if (lpn1->hwnd == lpn2->hwnd)
            return 0;
          else
            return -1;      
        }
      }
      break;
    case OT_MON_SERVER:
    case OT_MON_SERVERWITHSESS:
      {
        LPSERVERDATAMIN lps1 =(LPSERVERDATAMIN) pstruct1;
        LPSERVERDATAMIN lps2 =(LPSERVERDATAMIN) pstruct2;

//        ires = x_stricmp((char *)lps1->server_class,(char *)lps2->server_class);
        ires=cmplong((long)lps1->servertype,(long)lps2->servertype);
        if (ires!=0)
          return ires;
		if (lps1->servertype==-1) {
			ires = x_stricmp((char *)lps1->server_class,(char *)lps2->server_class);
			if (ires!=0)
				return ires;
		}
        if (bUpdSameObj) {
           lps1->bMultipleWithSameName=TRUE; 
           lps2->bMultipleWithSameName=TRUE;
        }
        return x_stricmp((char *)lps1->listen_address,(char *)lps2->listen_address);
      }
      break;
    case OT_MON_ACTIVE_USER:
      {        
        LPSESSIONDATAMIN lp1 = (LPSESSIONDATAMIN) pstruct1;
        LPSESSIONDATAMIN lp2 = (LPSESSIONDATAMIN) pstruct2;
        return x_stricmp((char *)lp1->effective_user,(char *)lp2->effective_user);
      }
      break;
    case OT_MON_SESSION:
	case OT_MON_BLOCKING_SESSION:
      {
        LPSESSIONDATAMIN lp1 = (LPSESSIONDATAMIN) pstruct1;
        LPSESSIONDATAMIN lp2 = (LPSESSIONDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->real_user,(char *)lp2->real_user);
        if (ires!=0)
          return ires;
        if (bUpdSameObj) {
           lp1->bMultipleWithSameName=TRUE;
           lp2->bMultipleWithSameName=TRUE;
        }
		ires = cmplong(lp1->server_pid,lp2->server_pid);
		if (ires)
			return ires;
        return x_stricmp(lp1->session_id,lp2->session_id);
      }
      break;
    case OT_MON_ICE_CONNECTED_USER      :
      {
        LPICE_CONNECTED_USERDATAMIN lp1 = (LPICE_CONNECTED_USERDATAMIN) pstruct1;
        LPICE_CONNECTED_USERDATAMIN lp2 = (LPICE_CONNECTED_USERDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->name,(char *)lp2->name);
        return ires;
      }
      break;
    case OT_MON_ICE_ACTIVE_USER         :
      {
        LPICE_ACTIVE_USERDATAMIN lp1 = (LPICE_ACTIVE_USERDATAMIN) pstruct1;
        LPICE_ACTIVE_USERDATAMIN lp2 = (LPICE_ACTIVE_USERDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->name,(char *)lp2->name);
        if (ires!=0)
          return ires;
        ires = x_stricmp((char *)lp1->host,(char *)lp2->host);
        if (ires!=0)
          return ires;  
        ires = x_stricmp((char *)lp1->query,(char *)lp2->query);
        return ires;
      }
      break;
    case OT_MON_ICE_TRANSACTION         :
      {
        LPICE_TRANSACTIONDATAMIN lp1 = (LPICE_TRANSACTIONDATAMIN) pstruct1;
        LPICE_TRANSACTIONDATAMIN lp2 = (LPICE_TRANSACTIONDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->trans_key,(char *)lp2->trans_key);
        return ires;
      }
      break;
    case OT_MON_ICE_CURSOR              :
      {
        LPICE_CURSORDATAMIN lp1 = (LPICE_CURSORDATAMIN) pstruct1;
        LPICE_CURSORDATAMIN lp2 = (LPICE_CURSORDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->curs_key,(char *)lp2->curs_key);
        return ires;
      }
      break;
    case OT_MON_ICE_FILEINFO            :
      {
        LPICE_CACHEINFODATAMIN lp1 = (LPICE_CACHEINFODATAMIN) pstruct1;
        LPICE_CACHEINFODATAMIN lp2 = (LPICE_CACHEINFODATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->name,(char *)lp2->name);
        if (ires!=0)
          return ires;
        ires = x_stricmp((char *)lp1->owner,(char *)lp2->owner);
        return ires;
      }
      break;
    case OT_MON_ICE_DB_CONNECTION       :
      {
        LPICE_DB_CONNECTIONDATAMIN lp1 = (LPICE_DB_CONNECTIONDATAMIN) pstruct1;
        LPICE_DB_CONNECTIONDATAMIN lp2 = (LPICE_DB_CONNECTIONDATAMIN) pstruct2;

        ires = x_stricmp((char *)lp1->dbname,(char *)lp2->dbname);
        if (ires!=0)
          return ires;
        ires = x_stricmp((char *)lp1->sessionkey,(char *)lp2->sessionkey);
        return ires;
      }
      break;

    case OT_MON_LOCKLIST:
      {
        LPLOCKLISTDATAMIN lp1 = (LPLOCKLISTDATAMIN) pstruct1;
        LPLOCKLISTDATAMIN lp2 = (LPLOCKLISTDATAMIN) pstruct2;
        if (lp1->bIs4AllLockLists  && lp2->bIs4AllLockLists)
           return 0;
        if (lp1->bIs4AllLockLists)  // is the last one in the list
           return (-1);
        if (lp2->bIs4AllLockLists)
           return 1;
        return cmplong(lp1->locklist_id,lp2->locklist_id);
      }
      break;
    case OT_DATABASE:
      {
        LPRESOURCEDATAMIN lp1 = (LPRESOURCEDATAMIN) pstruct1;
        LPRESOURCEDATAMIN lp2 = (LPRESOURCEDATAMIN) pstruct2;
        UCHAR DBName1[MAXOBJECTNAME];
        UCHAR DBName2[MAXOBJECTNAME];
        fstrncpy(DBName1,(LPUCHAR)GetMonDbNameFromKey(lp1->resource_key),sizeof(DBName1));
        fstrncpy(DBName2,(LPUCHAR)GetMonDbNameFromKey(lp2->resource_key),sizeof(DBName2));
        return x_stricmp((char *)DBName1,(char *)DBName2);
      }
      break;
    case OT_TABLE:
      {     // The 2 tables are supposed to be on the same database
        LPRESOURCEDATAMIN lp1 = (LPRESOURCEDATAMIN) pstruct1;
        LPRESOURCEDATAMIN lp2 = (LPRESOURCEDATAMIN) pstruct2;
        UCHAR tbName1[MAXOBJECTNAME];
        UCHAR tbName2[MAXOBJECTNAME];
        UCHAR tbowner1[MAXOBJECTNAME];
        UCHAR tbowner2[MAXOBJECTNAME];
        char *pDBName=GetMonDbNameFromID(lp1->hdl,lp1->resource_database_id);
        if (!pDBName)
           return 0;
        if (lp1->hdl!=lp2->hdl)
          myerror(ERR_DOMDATA);
        char *pc=GetMonTableName(lp1->hdl, (LPUCHAR)pDBName,(char *)tbowner1,
                                 lp1->resource_table_id);
        if (!pc)
            return 0;
        x_strcpy((char *)tbName1,pc);
        pc=GetMonTableName(lp1->hdl, (LPUCHAR)pDBName,(char *)tbowner2,
                                 lp2->resource_table_id);
        if (!pc)
            return 0;
        x_strcpy((char *)tbName2,pc);
        return DOMTreeCmpStr(tbName1,tbowner1,tbName2,tbowner2,
                               OT_TABLE,TRUE);
      }
      break;
    case OTLL_MON_RESOURCE:
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_MON_RES_OTHER:
      {
        LPRESOURCEDATAMIN lp1 = (LPRESOURCEDATAMIN) pstruct1;
        LPRESOURCEDATAMIN lp2 = (LPRESOURCEDATAMIN) pstruct2;
        if (lp1->res_database_name[0] && lp2->res_database_name[0]) {
           long tblid1,tblid2,pg1,pg2,idx1,idx2;
           ires=cmplong(lp1->resource_database_id,lp2->resource_database_id);
//           ires=x_stricmp(lp1->res_database_name,lp2->res_database_name);
           if (ires)
              return ires;  // different databases
           GetMonResourceTblPg(lp1, NULL, NULL, &tblid1, &idx1, &pg1);
           GetMonResourceTblPg(lp2, NULL, NULL, &tblid2, &idx2, &pg2);
           ires=cmplong(tblid1,tblid2);
           if (ires)
             return ires; //  includes situation where no table (-1)
           ires=cmplong(idx1,idx2);
           if (ires)
             return ires; //  includes situation where not an index (-1)
           ires=cmplong(pg1,pg2);
           if (ires)
             return ires; //  includes situation where no page  (-1)
        }
        if (lp1->res_database_name[0] && !lp2->res_database_name[0])
          return 1;
        if (!lp1->res_database_name[0] && lp2->res_database_name[0])
          return (-1);
        return cmplong(lp1->resource_id,lp2->resource_id);
      }
      break;
    case OT_MON_LOCK            :
    case OT_MON_LOCKLIST_LOCK   :
    case OT_MON_RES_LOCK        :
    case OT_MON_LOCKLIST_BL_LOCK:
      {
        LPLOCKDATAMIN lpl1 = (LPLOCKDATAMIN) pstruct1;
        LPLOCKDATAMIN lpl2 = (LPLOCKDATAMIN) pstruct2;
        return cmplong(lpl1->lock_id,lpl2->lock_id);
      }
      break;
    case OT_MON_LOGPROCESS:
      {
        LPLOGPROCESSDATAMIN lp1 = (LPLOGPROCESSDATAMIN) pstruct1;
        LPLOGPROCESSDATAMIN lp2 = (LPLOGPROCESSDATAMIN) pstruct2;
        ires = x_stricmp((char *)lp1->LogProcessName,(char *)lp2->LogProcessName);
        if (ires!=0)
          return ires;
        return cmplong(lp1->log_process_id,lp2->log_process_id);
      }
      break;
    case OT_MON_LOGDATABASE:
      {
        LPLOGDBDATAMIN lp1 = (LPLOGDBDATAMIN) pstruct1;
        LPLOGDBDATAMIN lp2 = (LPLOGDBDATAMIN) pstruct2;
        ires = x_stricmp((char *)lp1->db_name,(char *)lp2->db_name);
        return ires;
      }
      break;
    case OT_MON_REPLIC_SERVER:
      {
        LPREPLICSERVERDATAMIN lp1 = (LPREPLICSERVERDATAMIN) pstruct1;
        LPREPLICSERVERDATAMIN lp2 = (LPREPLICSERVERDATAMIN) pstruct2;
        ires = x_stricmp((char *)lp1->RunNode,(char *)lp2->RunNode);
		if (ires)
			return ires;
		ires=cmplong((long)(lp1->serverno), (long)lp2->serverno);
		if (ires)
			return ires;
		ires=cmplong((long)(lp1->localdb), (long)lp2->localdb);
	
        return ires;
      }
      break;
    case OT_MON_REPLIC_CDDS_ASSIGN:
      {
        LPREPLICCDDSASSIGNDATAMIN lp1 = (LPREPLICCDDSASSIGNDATAMIN) pstruct1;
        LPREPLICCDDSASSIGNDATAMIN lp2 = (LPREPLICCDDSASSIGNDATAMIN) pstruct2;
        ires=cmplong((long)lp1->Database_No,(long)lp2->Database_No);
        if (ires)
            return ires;
        ires=cmplong((long)lp1->Cdds_No,(long)lp2->Cdds_No);
        if (ires)
            return ires;
        ires = x_stricmp((char *)lp1->szDBName,(char *)lp2->szDBName);
        if (ires!=0)
            return ires;
        return x_stricmp((char *)lp1->szCddsName,(char *)lp2->szCddsName);
      }
      break;
    case OT_MON_TRANSACTION:
      {
        LPLOGTRANSACTDATAMIN lp1 = (LPLOGTRANSACTDATAMIN) pstruct1;
        LPLOGTRANSACTDATAMIN lp2 = (LPLOGTRANSACTDATAMIN) pstruct2;
        ires =  cmplong(lp1->tx_transaction_high,lp2->tx_transaction_high);
        if (ires!=0)
          return ires;
        return cmplong(lp1->tx_transaction_low,lp2->tx_transaction_low);
      }
      break;
    case OT_NODE_LOGINPSW:
      {
        LPNODELOGINDATAMIN lp1 = (LPNODELOGINDATAMIN) pstruct1;
        LPNODELOGINDATAMIN lp2 = (LPNODELOGINDATAMIN) pstruct2;
        if (lp1->bPrivate && !lp2->bPrivate)
          return 1;
        if (lp2->bPrivate && !lp1->bPrivate)
          return -1;
        ires = x_stricmp((char *)lp1->Login,(char *)lp2->Login);
        return ires;
      }
      break;
    case OT_NODE_ATTRIBUTE:
      {
        LPNODEATTRIBUTEDATAMIN lp1 = (LPNODEATTRIBUTEDATAMIN) pstruct1;
        LPNODEATTRIBUTEDATAMIN lp2 = (LPNODEATTRIBUTEDATAMIN) pstruct2;
 	    ires=lstrcmpi((const char *)lp1->AttributeName,(const char *)lp2->AttributeName);
		if (ires)
			return ires;
		ires=lstrcmpi((const char *)lp1->AttributeValue,(const char *)lp2->AttributeValue);
	    return ires;
      }
      break;
    case OT_NODE_CONNECTIONDATA:
      {
        LPNODECONNECTDATAMIN lp1 = (LPNODECONNECTDATAMIN) pstruct1;
        LPNODECONNECTDATAMIN lp2 = (LPNODECONNECTDATAMIN) pstruct2;
        if (lp1->bPrivate && !lp2->bPrivate)
          return 1;
        if (lp2->bPrivate && !lp1->bPrivate)
          return -1;
        if (lp1->ino>lp2->ino)
          return 1;
        if (lp1->ino==lp2->ino)
          return 0;
        return (-1);
      }
      break;
    case OT_LOCATION:
    case OT_LOCATIONWITHUSAGES:
    case OT_LOCATION_NODOUBLE:
      {
        LPLOCATIONDATAMIN lp1 = (LPLOCATIONDATAMIN) pstruct1;
        LPLOCATIONDATAMIN lp2 = (LPLOCATIONDATAMIN) pstruct2;
        return x_stricmp((char *)lp1->LocationName,(char *)lp2->LocationName);
      }
      break;
    case OT_REPLIC_REGTBLS_V11:
       {
        LPDD_REGISTERED_TABLES lp1 = (LPDD_REGISTERED_TABLES) pstruct1;
        LPDD_REGISTERED_TABLES lp2 = (LPDD_REGISTERED_TABLES) pstruct2;
        return DOMTreeCmpTableNames(lp1->tablename,lp1->tableowner,
                                    lp2->tablename,lp2->tableowner);
       }
      break;
    case OT_STARDB_COMPONENT:
       {
        LPSTARDBCOMPONENTPARAMS lp1 = (LPSTARDBCOMPONENTPARAMS) pstruct1;
        LPSTARDBCOMPONENTPARAMS lp2 = (LPSTARDBCOMPONENTPARAMS) pstruct2;
		int ires=lstrcmpi((const char *)lp1->NodeName,(const char *)lp2->NodeName);
		if (ires)
			return ires;
		ires=lstrcmpi((const char *)lp1->ServerClass,(const char *)lp2->ServerClass);
		if (ires)
			return ires;
		ires=lstrcmpi((const char *)lp1->DBName,(const char *)lp2->DBName);
		if (ires)
			return ires;
		if (lp1->itype!=lp2->itype) {
			int i1,i2;
			switch (lp1->itype) {
				case OT_TABLE    :i1=1;break;
				case OT_INDEX    :i1=2;break;
				case OT_VIEW     :i1=3;break;
				case OT_PROCEDURE:i1=4;break;
				default          :i1=0;break;
			}
			switch (lp2->itype) {
				case OT_TABLE    :i2=1;break;
				case OT_INDEX    :i2=2;break;
				case OT_VIEW     :i2=3;break;
				case OT_PROCEDURE:i2=4;break;
				default          :i2=0;break;
			}
			return (i1-i2);
		}
        return DOMTreeCmpTableNames(lp1->ObjectName,lp1->ObjectOwner,
                                    lp2->ObjectName,lp2->ObjectOwner);
       }
      break;

    default:
      {
        char name1[100];
        char name2[100];
        GetMonInfoName(0,iobjecttype, pstruct1, name1, sizeof(name1));
        GetMonInfoName(0,iobjecttype, pstruct2, name2, sizeof(name2));
        return x_strcmp((char *)name1, (char *)name2);
      }
      break;
  }
}

char *GetMonInfoName(int hdl, int iobjecttype, void *pstruct, char *buf,int bufsize)
{
  ASSERT(pstruct);
  char localbuf[300];
  char szInHex[30];
  char szInHex2[30];

  switch (iobjecttype) {
    case OT_NODE:
      {
        LPNODEDATAMIN lpn = (LPNODEDATAMIN) pstruct;
		if (lpn->bInstallPassword) 
			wsprintf(buf,"<installation password node>[%s]",lpn->NodeName);
		else
			fstrncpy((LPUCHAR)buf,lpn->NodeName,bufsize);
      }
      break;
    case OT_NODE_SERVERCLASS:
      {
        LPNODESVRCLASSDATAMIN lpn = (LPNODESVRCLASSDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,lpn->ServerClass,bufsize);
      }
      break;
 
    case OT_NODE_OPENWIN:
      {
        LPWINDOWDATAMIN lpn = (LPWINDOWDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,lpn->Title,bufsize);
      }
      break;
    case OT_MON_SERVER:
    case OT_MON_SERVERWITHSESS:
      {
        char *Display;
        //int ret;
        LPSERVERDATAMIN lps =(LPSERVERDATAMIN) pstruct;
        
        Display = FindServerTypeString(lps->servertype);
        if (Display)
           fstrncpy((LPUCHAR)localbuf,(LPUCHAR)Display,x_strlen(Display)+1);
        else 
           fstrncpy((LPUCHAR)localbuf,(LPUCHAR)lps->server_class,x_strlen(lps->server_class)+1);
#ifndef PROTOTYPE
        if (lps->bMultipleWithSameName) {
           x_strcat(localbuf," (");
           x_strcat(localbuf,lps->listen_address);
           //tafuncquiformattelenumeromaisplus tard mettreII1II2);
           x_strcat(localbuf,")");
        }
#endif

        fstrncpy((LPUCHAR)buf,(LPUCHAR)localbuf,bufsize);
      }
      break;
    case OT_MON_ACTIVE_USER:
      {        
        LPSESSIONDATAMIN lp = (LPSESSIONDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->effective_user,bufsize);
      }
      break;
    case OT_MON_SESSION:
	case OT_MON_BLOCKING_SESSION:
      {        
        LPSESSIONDATAMIN lp = (LPSESSIONDATAMIN) pstruct;
        fstrncpy((LPUCHAR)localbuf,(LPUCHAR)lp->real_user,MAXOBJECTNAME);
        if (x_strcmp(lp->effective_user,lp->real_user)) {
           x_strcat(localbuf,"/");
           x_strcat(localbuf,lp->effective_user);
        }
        if (lp->bMultipleWithSameName) {
           x_strcat(localbuf," (");
           x_strcat(localbuf,FormatHexaS(lp->session_id,szInHex));
           x_strcat(localbuf,")");
        }

        fstrncpy((LPUCHAR)buf,(LPUCHAR)localbuf,bufsize);

      }
      break;
    case OT_MON_ICE_CONNECTED_USER      :
      {
        LPICE_CONNECTED_USERDATAMIN lp = (LPICE_CONNECTED_USERDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->name,MAXOBJECTNAME);
      }
      break;
    case OT_MON_ICE_ACTIVE_USER         :
      {
        LPICE_ACTIVE_USERDATAMIN lp = (LPICE_ACTIVE_USERDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->name,MAXOBJECTNAME);
      }
      break;
    case OT_MON_ICE_TRANSACTION         :
      {
        LPICE_TRANSACTIONDATAMIN lp = (LPICE_TRANSACTIONDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->trans_key,MAXOBJECTNAME);
      }
      break;
    case OT_MON_ICE_CURSOR              :
      {
        LPICE_CURSORDATAMIN lp = (LPICE_CURSORDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->curs_key,MAXOBJECTNAME);
      }
      break;
    case OT_MON_ICE_FILEINFO            :
      {
        LPICE_CACHEINFODATAMIN lp = (LPICE_CACHEINFODATAMIN) pstruct;
        wsprintf(buf,"%s / %s",(LPUCHAR)lp->name,(LPUCHAR)lp->owner);
      }
      break;
    case OT_MON_ICE_DB_CONNECTION       :
      {
        LPICE_DB_CONNECTIONDATAMIN lp = (LPICE_DB_CONNECTIONDATAMIN) pstruct;
        wsprintf(buf,"%s / %s",(LPUCHAR)lp->dbname,(LPUCHAR)lp->sessionkey);
      }
      break;

    case OT_MON_LOCKLIST:
      {
        LPLOCKLISTDATAMIN lp = (LPLOCKLISTDATAMIN) pstruct;
        if (lp->locklist_lock_count == 0)
           x_strcpy(localbuf,"lock");
        else 
           x_strcpy(localbuf,"locks");
//        wsprintf(buf,"%s (%d %s)",FormatHexaL(lp->locklist_id,szInHex),lp->locklist_lock_count,localbuf);
        wsprintf(buf,"%s",FormatHexaL(lp->locklist_id,szInHex));
      }
      break;
    case OTLL_MON_RESOURCE:
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_MON_RES_OTHER:
    case OT_DATABASE:
    case OT_TABLE:
       {
        LPRESOURCEDATAMIN lp = (LPRESOURCEDATAMIN) pstruct;
        char *pDBName;
        char *pTableName;
        char tbowner[MAXOBJECTNAME];
        switch (lp->resource_type) {
          case RESTYPE_DATABASE:
            pDBName=GetMonDbNameFromKey(lp->resource_key);
            if ( pDBName) 
               fstrncpy((LPUCHAR)buf,(LPUCHAR)pDBName,bufsize);
            else 
               fstrncpy((LPUCHAR)buf,(LPUCHAR)"<system database>",bufsize);
            break;
          case RESTYPE_TABLE:
            pDBName=GetMonDbNameFromID(hdl, lp->resource_database_id);
            if (!pDBName)
               pDBName = "<system>";
            wsprintf(buf,"[%s]",(LPUCHAR)pDBName);
            pTableName=GetMonTableName(hdl,(LPUCHAR)pDBName,tbowner,lp->resource_table_id);
            if (!pTableName)  {
               wsprintf(localbuf,"<%ld,%ld>",lp->resource_table_id
                                              ,lp->resource_index_id);
               x_strcat(buf,localbuf);
            }
            else
               x_strcat(buf,pTableName);
            break;
          case RESTYPE_PAGE:
            pDBName=GetMonDbNameFromID(hdl, lp->resource_database_id);
            localbuf[0]='\0';
            if ( pDBName) {
               wsprintf(localbuf,"[%s]",pDBName);
               pTableName=GetMonTableName(hdl,(LPUCHAR)pDBName,tbowner,lp->resource_table_id);
               if (pTableName) {
                  x_strcat(localbuf,"[");
                  x_strcat(localbuf,pTableName);
                  x_strcat(localbuf,"]");
               }
            }
            wsprintf(buf,"%s / page %s%ld",FormatHexaL(lp->resource_id,szInHex),
                                          localbuf,
                                          lp->resource_page_number);
            break;
          default:
            wsprintf(buf,"%s / %s %s ",
                     FormatHexaL(lp->resource_id,szInHex),
                     FindResourceString(lp->resource_type),
                     FormatResourceDbTblPgInfo(lp, localbuf));
            break;
        }
       }
       break;
    case OT_MON_LOCK           :
    case OT_MON_RES_LOCK       :
    case OT_MON_LOCKLIST_LOCK  :
    case OT_MON_LOCKLIST_BL_LOCK:
       {
        LPLOCKDATAMIN lp = (LPLOCKDATAMIN) pstruct;
        char *pDBName;
        char *pTableName;
        char tbowner[MAXOBJECTNAME];
        switch (lp->resource_type) {
        case 1: // Database
           pDBName=GetMonDbNameFromKey(lp->resource_key);
           if ( pDBName) 
               wsprintf(buf,"%s / %s",FormatHexaL(lp->lock_id,szInHex),pDBName);
           else 
               wsprintf(buf,"%s / <unknown database>",FormatHexaL(lp->lock_id,szInHex));

           break;
        case 2: // table
           pDBName=GetMonDbNameFromID(hdl, lp->resource_database_id);
           if ( pDBName)
               x_strcpy(lp->res_database_name,pDBName);
           else {
               wsprintf(buf,"%s / [<unknown database>]",FormatHexaL(lp->lock_id,szInHex));
               break;
           }
           pTableName=GetMonTableName(hdl,(LPUCHAR)pDBName,tbowner,lp->resource_table_id);
           if (pTableName)
              wsprintf(buf,"%s / [%s]%s",FormatHexaL(lp->lock_id,szInHex),pDBName,pTableName);
           else
              wsprintf(buf,"%s / [%s]<unknown table>",FormatHexaL(lp->lock_id,szInHex),pDBName);
           break;
        case 3: // page
           pDBName=GetMonDbNameFromID(hdl, lp->resource_database_id);
           localbuf[0]='\0';
           if ( pDBName) {
               wsprintf(localbuf,"[%s]",pDBName);
               pTableName=GetMonTableName(hdl,(LPUCHAR)pDBName,tbowner,lp->resource_table_id);
               if (pTableName) {
                  x_strcat(localbuf,"[");
                  x_strcat(localbuf,pTableName);
                  x_strcat(localbuf,"]");
               }
           }
           wsprintf(buf,"%s / page %s%ld",FormatHexaL(lp->lock_id,szInHex),
                                         localbuf,
                                         lp->resource_page_number);
           break;
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
           {
           RESOURCEDATAMIN resdtaloc;
           lock2res(lp,&resdtaloc);
           wsprintf(buf,"%s / %s %s / %s",FormatHexaL(lp->lock_id,szInHex),
                                     FindResourceString(lp->resource_type),
                                     FormatHexaL(lp->resource_id,szInHex2),
                                     FormatResourceDbTblPgInfo(&resdtaloc, localbuf));
           }
           break;
        default :
           wsprintf(buf,"%s / <unknown type> %s",FormatHexaL(lp->lock_id,szInHex),
                                               FormatHexaL(lp->resource_id,szInHex2));
           break;
        }

       break;
       }
    case OT_MON_LOGPROCESS:
      {
        LPLOGPROCESSDATAMIN lp = (LPLOGPROCESSDATAMIN) pstruct;
        char buf1[200];
        int imax=bufsize-20;
        if (sizeof(buf1)<imax)
          imax=sizeof(buf1);
        fstrncpy((LPUCHAR)buf1,(LPUCHAR)lp->process_status,imax);
        wsprintf(buf,"%s / %s ",FormatHexaL(lp->log_process_id,szInHex),buf1);
      }
      break;
    case OT_MON_LOGDATABASE:
      {
        LPLOGDBDATAMIN lp = (LPLOGDBDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->db_name,bufsize);
      }
      break;
    case OT_MON_TRANSACTION:
      {
        LPLOGTRANSACTDATAMIN lp = (LPLOGTRANSACTDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->tx_transaction_id,bufsize);
      }
      break;
    case OT_MON_REPLIC_SERVER:
      {
        LPREPLICSERVERDATAMIN lp = (LPREPLICSERVERDATAMIN) pstruct;
		  wsprintf(buf,"%s:%d / ",lp->RunNode,lp->serverno);
        if (x_stricmp((char *)lp->RunNode,(char *)lp->LocalDBNode) != 0)    {
		      x_strcat(buf,(char *) lp->LocalDBNode);
            x_strcat(buf,"::");
        }
        x_strcat(buf,(char *)lp->LocalDBName);        
      }
      break;
    case OT_NODE_LOGINPSW:
      {
        LPNODELOGINDATAMIN lp = (LPNODELOGINDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->Login,bufsize);
      }
      break;
    case OT_NODE_CONNECTIONDATA:
      {
        LPNODECONNECTDATAMIN lp = (LPNODECONNECTDATAMIN) pstruct;
        fstrncpy((LPUCHAR)buf,(LPUCHAR)lp->Address,bufsize);
      }
      break;
    case OT_NODE_ATTRIBUTE:
      {
        LPNODEATTRIBUTEDATAMIN lp = (LPNODEATTRIBUTEDATAMIN) pstruct;
		wsprintf(buf,"%s : %s" ,lp->AttributeName,lp->AttributeValue);
      }
      break;
    case OT_LOCATION:
    case OT_LOCATIONWITHUSAGES:
    case OT_LOCATION_NODOUBLE:
      {
        LPLOCATIONDATAMIN lp = (LPLOCATIONDATAMIN) pstruct;
//        wsprintf(buf,"%s / %s",(char *)lp->LocationName,(char *)lp->LocationArea);
        wsprintf(buf,"%s",(char *)lp->LocationName);
      }
      break;
    case OT_USER:
      {
        LPNODEUSERDATAMIN lp = (LPNODEUSERDATAMIN) pstruct;
        wsprintf(buf,"%s",(char *)lp->User);
      }
      break;

    default:
      fstrncpy((LPUCHAR)buf,(LPUCHAR)pstruct,bufsize);
      break;
  }
  return buf;
}

int  GetMonInfoStructSize(int iobjecttype)
{
  
  switch (iobjecttype) {
    case OT_NODE:
      return sizeof(NODEDATAMIN);
      break;
    case OT_NODE_SERVERCLASS:
      return sizeof(NODESVRCLASSDATAMIN);
    case OT_NODE_OPENWIN:
      return sizeof(WINDOWDATAMIN);
      break;
    case OT_MON_SERVER:
    case OT_MON_SERVERWITHSESS:
      return sizeof(SERVERDATAMIN);
      break;
    case OT_MON_ACTIVE_USER:
    case OT_MON_SESSION:
	case OT_MON_BLOCKING_SESSION:
      return sizeof(SESSIONDATAMIN);
      break;
    case OT_MON_ICE_CONNECTED_USER      :
      return sizeof(ICE_CONNECTED_USERDATAMIN);
      break;
    case OT_MON_ICE_ACTIVE_USER         :
      return sizeof(ICE_ACTIVE_USERDATAMIN);
      break;
    case OT_MON_ICE_TRANSACTION         :
      return sizeof(ICE_TRANSACTIONDATAMIN);
      break;
    case OT_MON_ICE_CURSOR              :
      return sizeof(ICE_CURSORDATAMIN);
      break;
    case OT_MON_ICE_FILEINFO            :
      return sizeof(ICE_CACHEINFODATAMIN);
      break;
    case OT_MON_ICE_DB_CONNECTION       :
      return sizeof(ICE_DB_CONNECTIONDATAMIN);
      break;
    case OT_MON_LOCK           :
    case OT_MON_LOCKLIST_LOCK  :
    case OT_MON_RES_LOCK       :
    case OT_MON_LOCKLIST_BL_LOCK:
      return sizeof(LOCKDATAMIN);
      break;
    case OT_MON_LOCKLIST:
      return sizeof(LOCKLISTDATAMIN);
      break;
    case OTLL_MON_RESOURCE:
      return sizeof(RESOURCEDATAMIN);
      break;
    case OT_MON_LOGPROCESS:
      return sizeof(LOGPROCESSDATAMIN);
      break;
    case OT_MON_LOGDATABASE:
      return sizeof(LOGDBDATAMIN);
      break;
    case OT_MON_TRANSACTION:
      return sizeof(LOGTRANSACTDATAMIN);
      break;
    case OT_NODE_LOGINPSW:
      return sizeof(NODELOGINDATAMIN);
      break;
    case OT_NODE_CONNECTIONDATA:
      return sizeof(NODECONNECTDATAMIN);
      break;
    case OT_NODE_ATTRIBUTE:
      return sizeof(NODEATTRIBUTEDATAMIN);
      break;
    case OT_MON_REPLIC_SERVER:
      return sizeof(REPLICSERVERDATAMIN);
      break;
    case OT_MON_RES_DB:
    case OT_MON_RES_TABLE:
    case OT_MON_RES_PAGE:
    case OT_MON_RES_OTHER:
    case OT_DATABASE:
    case OT_TABLE:
      return sizeof(RESOURCEDATAMIN);
      break;
    case OT_LOCATION:
    case OT_LOCATIONWITHUSAGES:
    case OT_LOCATION_NODOUBLE:
      return sizeof(LOCATIONDATAMIN);
      break;
    case OT_USER:
      return sizeof(NODEUSERDATAMIN) ;
      break;
    default:
      myerror(ERR_MONITORTYPE);
      return sizeof(struct GeneralMonInfo);
      break;
  }
}

int MonCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype)
{
  if (*pnewtype != *poldtype) {
    switch (*poldtype) {
      case OT_TABLE:
        if (*pnewtype == OT_MON_RES_TABLE)
          return RES_SUCCESS;
        else
          return RES_ERR;
        break;

      case OT_MON_LOCKLIST_BL_LOCK:
      case OT_MON_LOCKLIST_LOCK:
      // Obsolete : case OT_MON_RES_LOCK:
        if (*pnewtype == OT_MON_LOCK)
          return RES_SUCCESS;
        else
          return RES_ERR;
        break;

      case OT_MON_LOCATION_DB:
//      case OT_MON_LOGDATABASE:
      case OT_MON_RES_DB:
        if (*pnewtype == OT_DATABASE)
          return RES_SUCCESS;
        else
          return RES_ERR;
        break;

      case OT_MON_DB_SERVER:
      case OT_MON_LOCK_SERVER:
        if (*pnewtype == OT_MON_SERVER)
          return RES_SUCCESS;
        else
          return RES_ERR;
        break;

      case OT_MON_LOCK_SESSION:
        if (*pnewtype == OT_MON_SESSION)
          return RES_SUCCESS;
        else
          return RES_ERR;
        break;

      case OTLL_MON_RESOURCE:
        {
        LPRESOURCEDATAMIN lpresdta = (LPRESOURCEDATAMIN)pstruct;
          switch (lpresdta->resource_type) {
            case RESTYPE_DATABASE:
              *pnewtype=OT_MON_RES_DB;
              break;
            case RESTYPE_TABLE:
              *pnewtype=OT_MON_RES_TABLE;
              break;
            case RESTYPE_PAGE:
              *pnewtype=OT_MON_RES_PAGE;
              break;
            default:
              *pnewtype=OT_MON_RES_OTHER;
              break;
          }
          return RES_SUCCESS;
        }
        break;

      case OT_MON_BLOCKING_SESSION:
        if (*pnewtype == OT_MON_SESSION) {
		  *poldtype = OT_MON_SESSION;	// MUST update type used for right pane detail/list properties
          return RES_SUCCESS;
		}
        else
          return RES_ERR;
        break;
    }
  }

  if (*pnewtype == OT_TOBESOLVED)
    return RES_ERR;

  if (*pnewtype != *poldtype)
    return RES_ERR;

//  testersizeofs

  return RES_SUCCESS;
}

static int GetMonServerMaxCurrentSession( int Curhnode , LPSERVERDATAMAX lpserver)
{
   int ires;
   SESSIONDATAMIN sessiondta;
   SERVERDATAMIN  serverdta = (SERVERDATAMIN)lpserver->serverdatamin;

   lpserver->current_connections=0;
   lpserver->active_current_sessions=0;
   push4mong();
   ires=GetFirstMonInfo(Curhnode,OT_MON_SERVER,&serverdta,
                        OT_MON_SESSION,&sessiondta,NULL);
   while (ires == RES_SUCCESS)  {
      lpserver->current_connections++;
      if (sessiondta.session_state_num == 1 )    // COMPUTABLE_STATE) // 1
         lpserver->active_current_sessions++;
      ires=GetNextMonInfo(&sessiondta);
   } 
   pop4mong();
   if (ires==RES_ENDOFDATA)
      ires=RES_SUCCESS;
   return ires;
}

int MonGetDetailInfo(int hnode, void *pbigstruct, int oldtype, int newtype) 
{
	return Task_MonGetDetailInfoInterruptible (hnode, pbigstruct, oldtype, newtype);
}

int MonGetDetailInfoLL(int hnode, void *pbigstruct, int oldtype, int newtype)
{
  int iret,ilocsession;
  BOOL ActiveConnect = FALSE;
  char buftemp[MAXOBJECTNAME];

  switch (oldtype) {
    case OT_MON_SERVER :
      {
        LPSERVERDATAMAX lpServerDataMax;

        lpServerDataMax= (LPSERVERDATAMAX)pbigstruct;

        if (x_strcmp(lpServerDataMax->serverdatamin.server_class,"INGRES") == 0 ||
            x_strcmp(lpServerDataMax->serverdatamin.server_class,"STAR")   == 0 ||
            x_strcmp(lpServerDataMax->serverdatamin.server_class,"IUSVR")  == 0 ) {
            iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
            
            if (iret !=RES_SUCCESS)
              return RES_ERR;
            ActiveConnect = TRUE;

            iret = MonitorGetAllServerInfo(lpServerDataMax);
            if (lpServerDataMax->current_connections == -1  &&
                lpServerDataMax->current_max_connections == -1  &&
                lpServerDataMax->active_current_sessions == -1  &&
                lpServerDataMax->active_max_sessions == -1 ) {   
            
                   iret = MonitorGetServerInfo(lpServerDataMax);
                   iret = GetMonServerMaxCurrentSession( hnode , lpServerDataMax);
                   lpServerDataMax->current_max_connections = -1;
            
            }
        } else {
            lpServerDataMax->current_connections      = -1;
            lpServerDataMax->current_max_connections  = -1;
            lpServerDataMax->active_current_sessions  = -1;
            lpServerDataMax->active_max_sessions      = -1 ;
            iret = RES_SUCCESS;
        }
      }
    break;
    case OT_MON_SESSION :
      {
        LPSESSIONDATAMAX lpSessionDataMax = (LPSESSIONDATAMAX)pbigstruct;
        iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);

        if (iret !=RES_SUCCESS)
          return RES_ERR;
        ActiveConnect = TRUE;

        iret = MonitorGetDetailSessionInfo(lpSessionDataMax);
      }
    break;
    case OTLL_MON_RESOURCE:
      {
        int ires;
        LPRESOURCEDATAMAX lpRscDataMax = (LPRESOURCEDATAMAX)pbigstruct;
        LOCKDATAMIN   lockdta;

        lpRscDataMax->number_locks=0;

        push4mong();
        ires = GetFirstMonInfo(hnode,OTLL_MON_RESOURCE,&lpRscDataMax->resourcedatamin,
                               OT_MON_LOCK,&lockdta,NULL);
        while (ires == RES_SUCCESS)  {
            lpRscDataMax->number_locks++;
            ires = GetNextMonInfo(&lockdta);
        }
        pop4mong();
        if (ires==RES_ENDOFDATA)
          ires=RES_SUCCESS;

        x_strcpy(lpRscDataMax->resourcedatamin.res_table_name,
               GetResTableNameStr(&(lpRscDataMax->resourcedatamin),buftemp,sizeof(buftemp)));
        return ires;
      }
      break;
    case OT_MON_TRANSACTION:
      {
      LPLOGTRANSACTDATAMAX lpTransactionDataMax = (LPLOGTRANSACTDATAMAX)pbigstruct;
      iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);

      if (iret !=RES_SUCCESS)
         return RES_ERR;
      ActiveConnect = TRUE;

      iret = MonitorGetDetailTransactionInfo(lpTransactionDataMax);
      }
      break;

    case OT_MON_LOGPROCESS:
      iret = RES_SUCCESS;
      break;
    case OT_MON_RES_LOCK:
    case OT_MON_LOCK:
      {
        LPLOCKDATAMAX lplockdta = (LPLOCKDATAMAX) pbigstruct;
        x_strcpy(lplockdta->lockdatamin.res_table_name,
               GetLockTableNameStr(&(lplockdta->lockdatamin),buftemp,sizeof(buftemp)));
        iret = RES_SUCCESS;
      }
      break;
    case OT_MON_LOCKLIST:
      {
        LPLOCKLISTDATAMAX lplocklistdta = (LPLOCKLISTDATAMAX) pbigstruct;
        
        iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
        if (iret !=RES_SUCCESS)
          return RES_ERR;

        ActiveConnect = TRUE;
        iret = MonitorGetDetailLockListInfo(lplocklistdta);
        GetTransactionString(hnode, lplocklistdta->locklistdatamin.locklist_tx_high,
                                    lplocklistdta->locklistdatamin.locklist_tx_low,
                                    lplocklistdta->locklistdatamin.locklist_transaction_id,
                                    sizeof(lplocklistdta->locklistdatamin.locklist_transaction_id));
        lplocklistdta->WaitingResource [0]='\0';
        if (iret==RES_SUCCESS) {
           LOCKDATAMIN   lockdta;
           if (lplocklistdta->locklistdatamin.locklist_wait_id) {
              push4mong();
              iret = GetFirstMonInfo(hnode,0,NULL,OT_MON_LOCK,&lockdta,NULL);
              while (iret == RES_SUCCESS)  {
                  if (lockdta.lock_id==lplocklistdta->locklistdatamin.locklist_wait_id) {
                    FormatHexaL( lockdta.resource_id,lplocklistdta->WaitingResource);
                    break;
                  }
                  iret = GetNextMonInfo(&lockdta);
              }
              pop4mong();
           }
        }
      }
    break;
    case OT_MON_LOCKINFO :
      {
        LPLOCKSUMMARYDATAMIN LockSummary = (LPLOCKSUMMARYDATAMIN)pbigstruct; 
        
        iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
        if (iret !=RES_SUCCESS)
          return RES_ERR;

        ActiveConnect = TRUE;
        iret = MonitorGetDetailLockSummary( LockSummary );
      }
    break;
    case OT_MON_LOGINFO:
      {
        LPLOGSUMMARYDATAMIN pLogSummary = (LPLOGSUMMARYDATAMIN)pbigstruct;
        
        iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
        if (iret !=RES_SUCCESS)
          return RES_ERR;

        ActiveConnect = TRUE;
        iret = MonitorGetDetailLogSummary( pLogSummary );

      }
    break;
    case OT_MON_LOGHEADER:
      {
        LPLOGHEADERDATAMIN pLogHeader = (LPLOGHEADERDATAMIN)pbigstruct;
        
        iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
        if (iret !=RES_SUCCESS)
          return RES_ERR;

        ActiveConnect = TRUE;
        iret = MonitorGetDetailLogHeader( pLogHeader );

      }
    break;
    default :
      {
      char buffaff[50];
      wsprintf(buffaff,VDBA_MfcResourceString (IDS_F_DETAIL_INFO),oldtype);//"no detail info for type number: %d."
      MessageBox(GetFocus(), buffaff,
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      iret = RES_ERR;
      }
  }
  if (ActiveConnect == TRUE)  {
    if (iret==RES_SUCCESS ) 
      iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
    else 
      ReleaseSession(ilocsession, RELEASE_ROLLBACK);
  }
  return iret;
}
static void sublocksummary(LPLOCKSUMMARYDATAMIN presult,
                           LPLOCKSUMMARYDATAMIN pstart)
{
  // Lock list information     
  presult->sumlck_lklist_created_list -=pstart->sumlck_lklist_created_list;        
  presult->sumlck_lklist_released_all -=pstart->sumlck_lklist_released_all;        
//  presult->sumlck_lklist_inuse        -=pstart->sumlck_lklist_inuse;               
//  presult->sumlck_lklist_remaining    -=pstart->sumlck_lklist_remaining;           
//  presult->sumlck_total_lock_lists_available-=pstart->sumlck_total_lock_lists_available; 

  // Hash Table sizes                          
//  presult->sumlck_rsh_size         -=pstart->sumlck_rsh_size;          
//  presult->sumlck_lkh_size         -=pstart->sumlck_lkh_size;          

  // Lock information                 
  presult->sumlck_lkd_request_new  -=pstart->sumlck_lkd_request_new;   
  presult->sumlck_lkd_request_cvt  -=pstart->sumlck_lkd_request_cvt;   
  presult->sumlck_lkd_convert      -=pstart->sumlck_lkd_convert;       
  presult->sumlck_lkd_release      -=pstart->sumlck_lkd_release;       
  presult->sumlck_lkd_cancel       -=pstart->sumlck_lkd_cancel;        
  presult->sumlck_lkd_rlse_partial -=pstart->sumlck_lkd_rlse_partial;  
//  presult->sumlck_lkd_inuse        -=pstart->sumlck_lkd_inuse;         
//  presult->sumlck_locks_remaining  -=pstart->sumlck_locks_remaining;   
//  presult->sumlck_locks_available  -=pstart->sumlck_locks_available;   

  presult->sumlck_lkd_dlock_srch   -=pstart->sumlck_lkd_dlock_srch;    
  presult->sumlck_lkd_cvt_deadlock -=pstart->sumlck_lkd_cvt_deadlock;  
  presult->sumlck_lkd_deadlock     -=pstart->sumlck_lkd_deadlock;      

  presult->sumlck_lkd_convert_wait -=pstart->sumlck_lkd_convert_wait;  
  presult->sumlck_lkd_wait         -=pstart->sumlck_lkd_wait;          

//  presult->sumlck_lkd_max_lkb      -=pstart->sumlck_lkd_max_lkb;       
  
//  presult->sumlck_lkd_rsb_inuse    -=pstart->sumlck_lkd_rsb_inuse;     
}


static void sublogsummary(LPLOGSUMMARYDATAMIN presult,
                          LPLOGSUMMARYDATAMIN pstart)
{
    presult->lgd_add           -=pstart->lgd_add;          
    presult->lgd_remove        -=pstart->lgd_remove;
    presult->lgd_begin         -=pstart->lgd_begin;        
    presult->lgd_Present_end    =presult->lgd_end;      // needed to calculate 
    presult->lgd_Present_writeio=presult->lgd_writeio;  // needed to calculate 
    presult->lgd_end           -=pstart->lgd_end;
    presult->lgd_write         -=pstart->lgd_write;        
    presult->lgd_writeio       -=pstart->lgd_writeio;
    presult->lgd_readio        -=pstart->lgd_readio;
    presult->lgd_wait          -=pstart->lgd_wait;         
    presult->lgd_split         -=pstart->lgd_split;
    presult->lgd_free_wait     -=pstart->lgd_free_wait;
    presult->lgd_stall_wait    -=pstart->lgd_stall_wait;
    presult->lgd_force         -=pstart->lgd_force;        
    presult->lgd_group_force   -=pstart->lgd_group_force;  
    presult->lgd_group_count   -=pstart->lgd_group_count;  
    presult->lgd_pgyback_check -=pstart->lgd_pgyback_check;
    presult->lgd_pgyback_write -=pstart->lgd_pgyback_write;
    presult->lgd_kbytes        -=pstart->lgd_kbytes;       
    presult->lgd_inconsist_db  -=pstart->lgd_inconsist_db; 

}

int MonGetLockSummaryInfo(int hnode,
                          LPLOCKSUMMARYDATAMIN presult,
                          LPLOCKSUMMARYDATAMIN pstart,
                          BOOL bSinceStartUp,
                          BOOL bNowAction)
{
  int iret,ilocsession;

  iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS)
    return RES_ERR;

  iret = MonitorGetDetailLockSummary(presult);
  if (iret==RES_SUCCESS ) 
    iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
  else 
    ReleaseSession(ilocsession, RELEASE_ROLLBACK);
  if (bNowAction) 
    *pstart=*presult;
  if (!bSinceStartUp)
     sublocksummary(presult,pstart);
  return iret;
}

int MonGetLogSummaryInfo(int hnode,
                          LPLOGSUMMARYDATAMIN presult,
                          LPLOGSUMMARYDATAMIN pstart,
                          BOOL bSinceStartUp,
                          BOOL bNowAction)
{
  int iret,ilocsession;

  iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS)
    return RES_ERR;

  iret = MonitorGetDetailLogSummary(presult);
  if (iret==RES_SUCCESS ) 
    iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
  else 
    ReleaseSession(ilocsession, RELEASE_ROLLBACK);
  if (bNowAction) 
    *pstart=*presult;
  if (!bSinceStartUp)
     sublogsummary(presult,pstart);
  return iret;
}

int MonShutDownSvr(int hnode, LPSERVERDATAMIN pServer, BOOL bSoft)
{
  int iret,ilocsession;
  int itype = bSoft?SHUTDOWN_SOFT:SHUTDOWN_HARD;

  if (!ExistShutDownSvr(hnode)) {
    //"Shutdown Server is not available for this server"
    MessageBox(GetFocus(), VDBA_MfcResourceString(IDS_E_SHUTDOWN_SVR),
               NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return RES_ERR;
  }

  iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_INDEPENDENT, &ilocsession);
  if (iret !=RES_SUCCESS)
    return RES_ERR;


  iret = DBA_ShutdownServer (pServer , itype);
     ReleaseSession(ilocsession, RELEASE_COMMIT);
//  if (iret==RES_SUCCESS)
//    iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
//  else 
//    ReleaseSession(ilocsession, RELEASE_ROLLBACK);
  return RES_SUCCESS;
}

int MonRemoveSession(int hnode, LPSESSIONDATAMIN pSession)
{
  int iret,ilocsession,ireturn;
  char CurID[MAXOBJECTNAME];
  char test[MAXOBJECTNAME];
  long sesid;

  suppspace((LPUCHAR)pSession->sesssvrdata.name_server);

  if (!ExistRemoveSessionProc(hnode)) {
      //"Remove session not available on this server"
      MessageBox(GetFocus(), VDBA_MfcResourceString (IDS_E_REMOVE_SESS),
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
     return RES_ERR;
  }

  iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
  if (iret !=RES_SUCCESS)
    return RES_ERR;


  iret = MonitorFindCurrentSessionId( CurID );
  if (iret!=RES_SUCCESS)  {
    ReleaseSession(ilocsession, RELEASE_ROLLBACK);
    return iret;
  }

  sesid=atol(pSession->session_id);
  wsprintf(test,"%08lX",sesid);   // uppercase for hexa needed

  if ( x_strcmp(CurID,test) == 0) {
    //"This session is the session you are currently using. Remove it anyway ?"
    ireturn = MessageBox(GetFocus(),VDBA_MfcResourceString (IDS_E_CURRENT_SESSION),
               VDBA_MfcResourceString(IDS_T_WARNING), MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL);

    if ( ireturn == IDCANCEL)  {
      ReleaseSession(ilocsession, RELEASE_ROLLBACK);
      return RES_SUCCESS;
    }
  }

  iret = DBA_RemoveSession ( pSession ,pSession->sesssvrdata.listen_address);
  if (iret==RES_SUCCESS)
    iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
  else 
    ReleaseSession(ilocsession, RELEASE_ROLLBACK);
  return iret;
}

BOOL IsMonTrueParent(int iobjecttypeparent, int iobjecttypeobj)
{
//struct parentequiv {
//      int   iptype;
//      int   iotype;
//}   petbl[] = {
//      {OT_MON_SESSION,OT_MON_LOCKLIST},
//      {0,0}
//    } ;
//  struct parentequiv *p1 =petbl;
//  while (p1->iptype && p1->iotype) {
//     if (p1->iotype==iobjecttypeobj) {
//        if (p1->iptype==iobjecttypeparent)
//           return TRUE;
//        return FALSE;
//     }
//     p1++;
//  }
  return TRUE;
}

int *MonGetDependentTypeList(int iobjecttype)
{
  static int aint[20];
  aint[0] = 0;
  return aint;
}

BOOL ContinueExpand(int iobjecttype)
{
  return FALSE;
}

int FindClassServerType(char *ServerClass, char * capabilities)
{
  struct ClassServerStruct *lpClassServerStruct;
  
  // Convert the class server structure string into the internal integer
  lpClassServerStruct = ClassServerKnown;
  while (*(lpClassServerStruct->lpValues)) {
     char * pc;
     BOOL bOK = FALSE;
     if (x_strcmp(lpClassServerStruct->lpValues[0],(char *) ServerClass)==0) {
        return lpClassServerStruct->nVal;         
     }
     pc = x_strstr(capabilities,lpClassServerStruct->lpValues[2]);
     if (pc) {
        char c = pc[x_strlen(lpClassServerStruct->lpValues[2])];
        if (c=='\0' || c==',') {
           if (pc>capabilities) {
             if (*(pc-1)==',') /* valid because ',' apparently can't be a trailing DBCS byte */
                 bOK = TRUE;   /* and pc is not the start of the buffer */
           }
           else 
               bOK = TRUE; /* capability at the beginning of the string */
        }
        if (bOK)
           return lpClassServerStruct->nVal;         
     }
     lpClassServerStruct++;
  }
return (-1);
}

char *FindServerTypeString(int svrtype)
{
  struct ClassServerStruct *lpClassServerStruct;
  
  // Convert the class server structure string into the internal string for display
  lpClassServerStruct = ClassServerKnown;
  while (*(lpClassServerStruct->lpValues)) {
      if (lpClassServerStruct->nVal == svrtype)
         return (lpClassServerStruct->lpValues[1]) ;
      lpClassServerStruct++;
  }
  return (NULL);
}


int MonOpenWindowsCount(int hNode)
{
  int   count = 0;
  HWND  hwndCurDoc;

  // get first document handle (with loop to skip the icon title windows)
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

  while (hwndCurDoc) {
    // test monitor on requested node
    if (QueryDocType(hwndCurDoc) == TYPEDOC_MONITOR)
      if (GetMDINodeHandle(hwndCurDoc) == hNode)
        count++;

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }

  return count;
}

int decodeStatusVal(int val,char *buf)
{
	*buf = '\0';

	if (val & LPB_MASTER) 
		x_strcat(buf,"RECOVER ");
	if (val & LPB_ARCHIVER) 
		x_strcat(buf,"ARCHIV ");
	if (val & LPB_FCT) 
		x_strcat(buf,"FCTDBMS ");
	if (val & LPB_RUNAWAY) 
		x_strcat(buf,"RUNAWAY ");
	if (val & LPB_SLAVE) 
		x_strcat(buf,"DBMS ");
	if (val & LPB_CKPDB) 
		x_strcat(buf,"CKPDB ");
	if (val & LPB_VOID) 
		x_strcat(buf,"VOID ");
	if (val & LPB_SHARED_BUFMGR) 
		x_strcat(buf,"SHR_BM ");
	if (val & LPB_IDLE) 
		x_strcat(buf,"IDLE ");
	if (val & LPB_DEAD) 
		x_strcat(buf,"DEAD ");
	if (val & LPB_DYING) 
		x_strcat(buf,"DYING ");
	//if (val & LPB_FOREIGN_RUNDOWN) 
	//	x_strcat(buf,"RUNDOWN ");
	//if (val & LPB_CPAGENT) 
	//	x_strcat(buf,"CPAGENT ");
	return(TRUE);
}

int MonOpenCloseSvr(int hnode, LPSERVERDATAMIN pServer, BOOL bOpen)
{
  int iret,ilocsession;

  if (!ExistProcedure4OpenCloseSvr( hnode)) {
    //"Procedures ima_open_server or ima_close_server\n"
    //"does not exist on this server."
    MessageBox(GetFocus(), VDBA_MfcResourceString(IDS_E_PROCEDURE_IMA),
               NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return RES_ERR;
  }

  iret = GetImaDBsession((LPUCHAR)GetVirtNodeName (hnode), SESSION_TYPE_INDEPENDENT, &ilocsession);
  if (iret !=RES_SUCCESS)
    return RES_ERR;

  iret = DBA_OpenCloseServer (pServer , bOpen);
  ReleaseSession(ilocsession, RELEASE_COMMIT);
  return iret;
}
