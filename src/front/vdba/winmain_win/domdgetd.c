/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Ingres Visual DBA
**
**  Source : domdgetd.c
**  Get information from cache
**
**  Author : Francois Noirot-Nerin
**
**  History after 01-01-2000
**
**  28-01-2000 (noifr01)
**  (bug 100196) when VDBA is invoked in the context, enforce a refresh
**  of the nodes list in the cache when the list is needed later than 
**  upon initialization
**  26-Apr-2000 (schph01)
**   (bug 101329) In the raw, cached list used both for getting the CDDS's
**   and the propagation paths, -1,-1 and -1 now indicate a CDDS with no path.
**   don't consider such elements as a propagation path.
**  20-May-2000 (noifr01)
**   (bug 99242) checked for DBCS compliance
**  06-Mar-2001 (schph01)
**   sir 97068 remove the code that was excluding QUEL views and store the
**   new parameter to know the language used to create the view.
**  16-Mar-2001 (schph01)
**   bug 104221 used the DOMTreeCmpTableNames() function for compare the object
**   name instead the x_strcmp() for the OT_VIEWTABLE
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  10-Nov-2004 (schph01)
**   (bug 113426) management of OT_ICE_MAIN.
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "dba.h"
#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"
#include "dbaset.h"
#include "dlgres.h"
#include "dll.h"
#include "monitor.h"


static BOOL bNodeListRequiresSpecialRefresh = FALSE;
void InitContextNodeListStatus()
{
    bNodeListRequiresSpecialRefresh = FALSE;
	return;
}
void RefreshNodesListIfNeededUponNextFetch()
{
	if (isNodesListInSpecialState())
		bNodeListRequiresSpecialRefresh = TRUE;

	return;
}
struct ResultReverseInfos {
   UCHAR ObjectName [MAXOBJECTNAME];
   UCHAR ObjectOwner[MAXOBJECTNAME];
   UCHAR ExtraData[EXTRADATASIZE];
   UCHAR Parents[MAXPLEVEL][MAXOBJECTNAME];

   struct ResultReverseInfos * pnext;
};

static char * lperrusr = "<Unavailable List>";


static int iObject;
static int iobjecttypemem;
static BOOL bsystemmem;
static void *pmemstruct;
static int nbelemmem;
static char lpownermem   [MAXOBJECTNAME];
static char lpllfiltermem[MAXOBJECTNAME];
static char par0         [MAXOBJECTNAME];
static int  hnodemem;
static int  lltypemem;


#define MAXLOOP 8


struct type_stack4domg {
   int    iObject;
   int    iobjecttypemem;
   BOOL   bsystemmem;
   void * pmemstruct;
   int    nbelemmem;
   char   lpownermem   [MAXOBJECTNAME];
   char   lpllfiltermem[MAXOBJECTNAME];
   char   par0         [MAXOBJECTNAME];
   int    hnodemem;
   int    lltypemem;
};

struct type_stack4domg stack4domg[MAXLOOP];
static int stacklevel = 0;

void push4domg ()
{
   if (stacklevel>=MAXLOOP) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   stack4domg[stacklevel].iObject        = iObject        ;
   stack4domg[stacklevel].iobjecttypemem = iobjecttypemem ;
   stack4domg[stacklevel].bsystemmem     = bsystemmem     ;
   stack4domg[stacklevel].pmemstruct     = pmemstruct     ;
   stack4domg[stacklevel].nbelemmem      = nbelemmem      ;
   fstrncpy(stack4domg[stacklevel].lpownermem,   lpownermem ,  MAXOBJECTNAME);
   fstrncpy(stack4domg[stacklevel].lpllfiltermem,lpllfiltermem,MAXOBJECTNAME);
   fstrncpy(stack4domg[stacklevel].par0,         par0,         MAXOBJECTNAME);
   stack4domg[stacklevel].hnodemem       = hnodemem       ;
   stack4domg[stacklevel].lltypemem      = lltypemem      ;
   stacklevel++;
}

void pop4domg ()
{
   stacklevel--;
   if (stacklevel<0) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   iObject        = stack4domg[stacklevel].iObject        ;
   iobjecttypemem = stack4domg[stacklevel].iobjecttypemem ;
   bsystemmem     = stack4domg[stacklevel].bsystemmem     ;
   pmemstruct     = stack4domg[stacklevel].pmemstruct     ;
   nbelemmem      = stack4domg[stacklevel].nbelemmem      ;
   fstrncpy(lpownermem ,  stack4domg[stacklevel].lpownermem,   MAXOBJECTNAME);
   fstrncpy(lpllfiltermem,stack4domg[stacklevel].lpllfiltermem,MAXOBJECTNAME);
   fstrncpy(par0,         stack4domg[stacklevel].par0,         MAXOBJECTNAME);
   hnodemem       = stack4domg[stacklevel].hnodemem       ;
   lltypemem      = stack4domg[stacklevel].lltypemem      ;
}


int  DOMGetFirstObject  ( hnodestruct, iobjecttype, level, parentstrings,
                          bWithSystem, lpowner, lpobjectname, lpresultowner,
                          lpresultextrastring)
int hnodestruct;
int iobjecttype;
int level;
LPUCHAR *parentstrings;
BOOL bWithSystem;
LPUCHAR lpowner;
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;
LPUCHAR lpresultextrastring;
{
   int   i, ires;

   struct ServConnectData * pdata1;

    switch (iobjecttype) {
      case OT_ICE_ROLE                 :
      case OT_ICE_DBUSER               :
      case OT_ICE_DBCONNECTION         :
      case OT_ICE_WEBUSER              :
      case OT_ICE_WEBUSER_ROLE         :
      case OT_ICE_WEBUSER_CONNECTION   :
      case OT_ICE_PROFILE              :
      case OT_ICE_PROFILE_ROLE         :
      case OT_ICE_PROFILE_CONNECTION   :
      case OT_ICE_BUNIT                :
      case OT_ICE_BUNIT_SEC_ROLE       :
      case OT_ICE_BUNIT_SEC_USER       :
      case OT_ICE_BUNIT_FACET          :
      case OT_ICE_BUNIT_PAGE           :
      case OT_ICE_BUNIT_LOCATION       :
      case OT_ICE_SERVER_APPLICATION   :
      case OT_ICE_SERVER_LOCATION      :
      case OT_ICE_SERVER_VARIABLE      :
		{
				char buf1[100],buf2[100];
				if (GetICeUserAndPassWord(hnodestruct,buf1, buf2)!=RES_SUCCESS)
					return RES_ERR;
		}

    }


   // TEMPORAIRE EMB - FINIR FNN : GESTION TYPE PLUS FILTRAGE SUR 2e PARENT = NOM DU SCHEMA
   if (iobjecttype == OT_SCHEMAUSER_TABLE || iobjecttype == OT_SCHEMAUSER_VIEW
                                          || iobjecttype == OT_SCHEMAUSER_PROCEDURE) {
     if (iobjecttype == OT_SCHEMAUSER_TABLE)
       iobjecttype = OT_TABLE, level = 1;
     if (iobjecttype == OT_SCHEMAUSER_VIEW)
       iobjecttype = OT_VIEW, level = 1;
     if (iobjecttype == OT_SCHEMAUSER_PROCEDURE)
       iobjecttype = OT_PROCEDURE, level = 1;
     return DOMGetFirstObject(hnodestruct, iobjecttype, level, parentstrings, bWithSystem,
                              parentstrings[1], // used as lpowner ---> will act as the filter, whatever the original lpowner was
                              lpobjectname, lpresultowner, lpresultextrastring);
   }
   // FIN TEMPORAIRE EMB

   iobjecttypemem    = iobjecttype;
   bsystemmem        = bWithSystem;
   hnodemem          = hnodestruct;

   par0[0]='\0';
   if (level && parentstrings)  {
      if (parentstrings[0])
         fstrncpy(par0,parentstrings[0], MAXOBJECTNAME);
   }

   if (lpowner) {
      if (x_stricmp(lpowner,lperrusr))
         fstrncpy(lpownermem,lpowner, MAXOBJECTNAME);
      else 
         lpownermem[0]='\0';
   }
   else
      lpownermem[0]='\0';
   lpllfiltermem[0]='\0';
   lltypemem=0;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1 && iobjecttype!=OT_NODE
			   && iobjecttype!=OT_NODE_OPENWIN
               && iobjecttype!=OT_NODE_SERVERCLASS
               && iobjecttype!=OT_NODE_LOGINPSW 
			   && iobjecttype!=OT_NODE_CONNECTIONDATA
			   && iobjecttype!=OT_NODE_ATTRIBUTE) 
      return myerror(ERR_UPDATEDOMDATA);

   /* load parent(s) if not done for "locate" */
   
   switch (iobjecttype) {
      case OT_TABLE:
      case OT_VIEW:
      case OT_VIEWALL:
      case OT_PROCEDURE:
      case OT_SEQUENCE:
      case OT_SCHEMAUSER:
      case OT_SYNONYM:
      case OT_DBEVENT:

      case OT_INDEX:
      case OT_INTEGRITY:
      case OT_RULE:
         if (!(pdata1->lpDBData)) {
            ires=UpdateDOMData(hnodestruct,OT_DATABASE,0,parentstrings,
                               bWithSystem,FALSE);
            if (ires != RES_SUCCESS)
               return ires;
         }
         break;
      case OT_MON_LOCKLIST_LOCK        :
         if (!(pdata1->lpLockListData)){
            ires=UpdateDOMData(hnodestruct,OT_MON_LOCKLIST,0,parentstrings,
                               bWithSystem,FALSE);
            if (ires != RES_SUCCESS)
               return ires;
         }
         break;
      case OT_MON_SESSION           :
      case OT_MON_ICE_CONNECTED_USER:
      case OT_MON_ICE_TRANSACTION   :
      case OT_MON_ICE_CURSOR        :
      case OT_MON_ICE_ACTIVE_USER   :
      case OT_MON_ICE_FILEINFO      :
      case OT_MON_ICE_DB_CONNECTION :

         if (!(pdata1->lpServerData)){
            ires=UpdateDOMData(hnodestruct,OT_MON_SERVER,0,parentstrings,
                               bWithSystem,FALSE);
            if (ires != RES_SUCCESS)
               return ires;
         }
         break;
      default:
         break;
   }

   switch (iobjecttype) {
      case OT_MON_SERVER         :
      case OT_MON_LOCKLIST       :
      case OTLL_MON_RESOURCE     :
      case OT_MON_LOGPROCESS     :
      case OT_MON_LOGDATABASE    :
      case OT_MON_TRANSACTION    :
      case OT_MON_SESSION        :
      case OT_MON_LOCKLIST_LOCK  :
	  case OT_MON_REPLIC_SERVER  :
      case OT_MON_ICE_CONNECTED_USER:
      case OT_MON_ICE_TRANSACTION   :
      case OT_MON_ICE_CURSOR        :
      case OT_MON_ICE_ACTIVE_USER   :
      case OT_MON_ICE_FILEINFO      :
      case OT_MON_ICE_DB_CONNECTION :

         if (!(pdata1->lpDBData)) {
            ires=UpdateDOMData(hnodestruct,OT_DATABASE,0,parentstrings,
                               bWithSystem,FALSE);
            if (ires != RES_SUCCESS)
               return ires;
         }
         break;
   }

   switch (iobjecttype) {

// Monitoring
      case OT_MON_SERVER         :
         {
            if (!pdata1->lpServerData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->serverserrcode != RES_SUCCESS)
               return pdata1->serverserrcode;
            pmemstruct = (void *) pdata1->lpServerData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbservers;
            break;
         }
      case OT_MON_LOCKLIST       :
         {
            if (!pdata1->lpLockListData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->LockListserrcode != RES_SUCCESS)
               return pdata1->LockListserrcode;
            pmemstruct = (void *) pdata1->lpLockListData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nblocklists;
            break;
         }
      case OTLL_MON_RESOURCE       :
         {
            if (!pdata1->lpResourceData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->resourceserrcode != RES_SUCCESS)
               return pdata1->resourceserrcode;
            pmemstruct = (void *) pdata1->lpResourceData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbresources;
            break;
         }
      case OT_MON_LOGPROCESS     :
         {
            if (!pdata1->lpLogProcessData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->logprocesseserrcode != RES_SUCCESS)
               return pdata1->logprocesseserrcode;
            pmemstruct = (void *) pdata1->lpLogProcessData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbLogProcesses;
            break;
         }
      case OT_MON_LOGDATABASE    :
         {
            if (!pdata1->lpLogDBData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->logDBerrcode != RES_SUCCESS)
               return pdata1->logDBerrcode;
            pmemstruct = (void *) pdata1->lpLogDBData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbLogDB;
            break;
         }
      case OT_MON_TRANSACTION:
         {
            if (!pdata1->lpLogTransactData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->LogTransacterrcode != RES_SUCCESS)
               return pdata1->LogTransacterrcode;
            pmemstruct = (void *) pdata1->lpLogTransactData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbLogTransact;
            break;
         }
      case OT_MON_SESSION        :
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpServerSessionData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->ServerSessionserrcode != RES_SUCCESS)
                     return pServerdata->ServerSessionserrcode;
                  pmemstruct=(void *) pServerdata->lpServerSessionData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbServerSessions;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_CONNECTED_USER:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceConnUsersData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->iceConnuserserrcode != RES_SUCCESS)
                     return pServerdata->iceConnuserserrcode;
                  pmemstruct=(void *) pServerdata->lpIceConnUsersData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbiceconnusers;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_ACTIVE_USER:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceActiveUsersData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->iceactiveuserserrcode != RES_SUCCESS)
                     return pServerdata->iceactiveuserserrcode;
                  pmemstruct=(void *) pServerdata->lpIceActiveUsersData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbiceactiveusers;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_TRANSACTION:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceTransactionsData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icetransactionserrcode != RES_SUCCESS)
                     return pServerdata->icetransactionserrcode;
                  pmemstruct=(void *) pServerdata->lpIceTransactionsData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbicetransactions;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_CURSOR:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceCursorsData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icecursorsserrcode != RES_SUCCESS)
                     return pServerdata->icecursorsserrcode;
                  pmemstruct=(void *) pServerdata->lpIceCursorsData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbicecursors;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_FILEINFO:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceCacheInfoData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icecacheinfoerrcode != RES_SUCCESS)
                     return pServerdata->icecacheinfoerrcode;
                  pmemstruct=(void *) pServerdata->lpIceCacheInfoData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbicecacheinfo;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_DB_CONNECTION:
         {
            struct ServerData * pServerdata;

            pServerdata = pdata1->lpServerData;
            if (!pServerdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbservers;
                     i++,pServerdata=GetNextServer(pServerdata)) {
               if (!CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) {
                  if (!pServerdata->lpIceDbConnectData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icedbconnectinfoerrcode != RES_SUCCESS)
                     return pServerdata->icedbconnectinfoerrcode;
                  pmemstruct=(void *) pServerdata->lpIceDbConnectData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pServerdata->nbicedbconnectinfo;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;

      case OT_MON_LOCKLIST_LOCK        :
         {
            struct LockListData * pLockListdata;

            pLockListdata = pdata1->lpLockListData;
            if (!pLockListdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nblocklists;
                     i++,pLockListdata=GetNextLockList(pLockListdata)) {
               if (!CompareMonInfo(OT_MON_LOCKLIST, parentstrings,&(pLockListdata->LockListDta))) { 
                  if (!pLockListdata->lpLockData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pLockListdata->lockserrcode != RES_SUCCESS)
                     return pLockListdata->lockserrcode;
                  pmemstruct=(void *) pLockListdata->lpLockData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pLockListdata->nbLocks;
                  goto getobj;
               }
            }
            {
              LPLOCKLISTDATAMIN lpll = (LPLOCKLISTDATAMIN)parentstrings;
              if (lpll) {
                if (lpll->bIs4AllLockLists)
                  return RES_ERR;
              }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
//      case OT_MON_RES_LOCK   :
//         {
//            struct ResourceData * pResourcedata;
//
//            pResourcedata = pdata1->lpResourceData;
//            if (!pResourcedata) 
//               return myerror(ERR_DOMGETDATA);
//            for (i=0;i<pdata1->nbresources;
//                     i++,pResourcedata=GetNextResource(pResourcedata)) {
//               if (!x_strcmp(parentstrings[0],pResourcedata->ResourceDta.ResourceName)) {
//                  if (!pResourcedata->lpResourceLockData) {
//                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
//                                        parentstrings,bWithSystem,FALSE);
//                     if (ires != RES_SUCCESS)
//                        return ires;
//                  }
//                  if (pResourcedata->ResourceLockserrcode != RES_SUCCESS)
//                     return pResourcedata->ResourceLockserrcode;
//                  pmemstruct=(void *) pResourcedata->lpResourceLockData;
//                  if (!pmemstruct) 
//                     return myerror(ERR_UPDATEDOMDATA1);
//
//                  nbelemmem=pResourcedata->nbResourceLocks;
//                  goto getobj;
//               }
//            }
//            return myerror(ERR_PARENTNOEXIST);
//         }
//         break;
// end of monitoring
      case OT_NODE  :
         {
            if (!lpMonNodeData || bNodeListRequiresSpecialRefresh) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
			   if (bNodeListRequiresSpecialRefresh)
				   bNodeListRequiresSpecialRefresh=FALSE;
            }
            if (monnodeserrcode != RES_SUCCESS)
               return monnodeserrcode;
            pmemstruct = (void *) lpMonNodeData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=nbMonNodes;
            break;
         }
      case OT_NODE_OPENWIN :
         {
            struct MonNodeData * pmonnodedata;

            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<nbMonNodes;
                     i++,pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (!CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) {
                  if (!pmonnodedata->lpWindowData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pmonnodedata->windowserrcode != RES_SUCCESS)
                     return pmonnodedata->windowserrcode;
                  pmemstruct=(void *) pmonnodedata->lpWindowData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pmonnodedata->nbwindows;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_NODE_SERVERCLASS:
         {
            struct MonNodeData * pmonnodedata;

            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<nbMonNodes;
                     i++,pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (!CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) {
                  if (!pmonnodedata->lpServerClassData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pmonnodedata->serverclasseserrcode != RES_SUCCESS)
                     return pmonnodedata->serverclasseserrcode;
                  pmemstruct=(void *) pmonnodedata->lpServerClassData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pmonnodedata->nbserverclasses;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_NODE_LOGINPSW:
         {
            struct MonNodeData * pmonnodedata;

            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<nbMonNodes;
                     i++,pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (!CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) {
                  if (!pmonnodedata->lpNodeLoginData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pmonnodedata->loginerrcode != RES_SUCCESS)
                     return pmonnodedata->loginerrcode;
                  pmemstruct=(void *) pmonnodedata->lpNodeLoginData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pmonnodedata->nblogins;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_NODE_CONNECTIONDATA:
         {
            struct MonNodeData * pmonnodedata;

            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<nbMonNodes;
                     i++,pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (!CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) {
                  if (!pmonnodedata->lpNodeConnectionData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pmonnodedata->NodeConnectionerrcode != RES_SUCCESS)
                     return pmonnodedata->NodeConnectionerrcode;
                  pmemstruct=(void *) pmonnodedata->lpNodeConnectionData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pmonnodedata->nbNodeConnections;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_NODE_ATTRIBUTE:
         {
            struct MonNodeData * pmonnodedata;

            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<nbMonNodes;
                     i++,pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (!CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) {
                  if (!pmonnodedata->lpNodeAttributeData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pmonnodedata->NodeAttributeerrcode != RES_SUCCESS)
                     return pmonnodedata->NodeAttributeerrcode;
                  pmemstruct=(void *) pmonnodedata->lpNodeAttributeData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pmonnodedata->nbNodeAttributes;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_ICE_ROLE                 :
         {
            if (!pdata1->lpIceRolesData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->iceroleserrcode != RES_SUCCESS)
               return pdata1->iceroleserrcode;
            pmemstruct = (void *) pdata1->lpIceRolesData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbiceroles;
            break;
         }
      case OT_ICE_DBUSER               :
         {
            if (!pdata1->lpIceDbUsersData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icedbuserserrcode != RES_SUCCESS)
               return pdata1->icedbuserserrcode;
            pmemstruct = (void *) pdata1->lpIceDbUsersData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicedbusers;
            break;
         }
      case OT_ICE_DBCONNECTION         :
         {
            if (!pdata1->lpIceDbConnectionsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icedbconnectionserrcode != RES_SUCCESS)
               return pdata1->icedbconnectionserrcode;
            pmemstruct = (void *) pdata1->lpIceDbConnectionsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicedbconnections;
            break;
         }
      case OT_ICE_WEBUSER              :
         {
            if (!pdata1->lpIceWebUsersData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icewebusererrcode != RES_SUCCESS)
               return pdata1->icewebusererrcode;
            pmemstruct = (void *) pdata1->lpIceWebUsersData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicewebusers;
            break;
         }
      case OT_ICE_PROFILE              :
         {
            if (!pdata1->lpIceProfilesData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->iceprofileserrcode != RES_SUCCESS)
               return pdata1->iceprofileserrcode;
            pmemstruct = (void *) pdata1->lpIceProfilesData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbiceprofiles;
            break;
         }
      case OT_ICE_BUNIT                :
         {
            if (!pdata1->lpIceBusinessUnitsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icebusinessunitserrcode != RES_SUCCESS)
               return pdata1->icebusinessunitserrcode;
            pmemstruct = (void *) pdata1->lpIceBusinessUnitsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicebusinessunits;
            break;
         }
      case OT_ICE_SERVER_APPLICATION   :
         {
            if (!pdata1->lpIceApplicationsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->iceapplicationserrcode != RES_SUCCESS)
               return pdata1->iceapplicationserrcode;
            pmemstruct = (void *) pdata1->lpIceApplicationsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbiceapplications;
            break;
         }
      case OT_ICE_SERVER_LOCATION      :
         {
            if (!pdata1->lpIceLocationsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icelocationserrcode != RES_SUCCESS)
               return pdata1->icelocationserrcode;
            pmemstruct = (void *) pdata1->lpIceLocationsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicelocations;
            break;
         }
      case OT_ICE_SERVER_VARIABLE      :
         {
            if (!pdata1->lpIceVarsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->icevarserrcode != RES_SUCCESS)
               return pdata1->icevarserrcode;
            pmemstruct = (void *) pdata1->lpIceVarsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbicevars;
            break;
         }
      case OT_DATABASE  :
         {
            if (!pdata1->lpDBData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->DBerrcode != RES_SUCCESS)
               return pdata1->DBerrcode;
            pmemstruct = (void *) pdata1->lpDBData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbdb;
            break;
         }
      case OT_PROFILE   :
         {
            if (!pdata1->lpProfilesData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->profileserrcode != RES_SUCCESS)
               return pdata1->profileserrcode;
            pmemstruct = (void *) pdata1->lpProfilesData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbprofiles;
            break;
         }
      case OT_USER      :
         {
            if (!pdata1->lpUsersData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->userserrcode != RES_SUCCESS)
               return pdata1->userserrcode;
            pmemstruct = (void *) pdata1->lpUsersData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbusers;
            break;
         }
      case OT_GROUP     :
         {
            if (!pdata1->lpGroupsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->groupserrcode != RES_SUCCESS)
               return pdata1->groupserrcode;
            pmemstruct = (void *) pdata1->lpGroupsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbgroups;
            break;
         }
      case OT_GROUPUSER :
         {
            struct GroupData * pgroupdata;

            pgroupdata = pdata1->lpGroupsData;
            if (!pgroupdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbgroups;
                     i++,pgroupdata=GetNextGroup(pgroupdata)) {
               if (!x_strcmp(parentstrings[0],pgroupdata->GroupName)) {
                  if (!pgroupdata->lpGroupUserData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pgroupdata->groupuserserrcode != RES_SUCCESS)
                     return pgroupdata->groupuserserrcode;
                  pmemstruct=(void *) pgroupdata->lpGroupUserData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pgroupdata->nbgroupusers;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_ROLE      :
         {
            if (!pdata1->lpRolesData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->roleserrcode != RES_SUCCESS)
               return pdata1->roleserrcode;
            pmemstruct = (void *) pdata1->lpRolesData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nbroles;
            break;
         }
      case OT_ROLEGRANT_USER :
         {
            struct RoleData * pRoledata;

            pRoledata = pdata1->lpRolesData;
            if (!pRoledata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbroles;
                     i++,pRoledata=GetNextRole(pRoledata)) {
               if (!x_strcmp(parentstrings[0],pRoledata->RoleName)) {
                  if (!pRoledata->lpRoleGranteeData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pRoledata->rolegranteeserrcode != RES_SUCCESS)
                     return pRoledata->rolegranteeserrcode;
                  pmemstruct=(void *) pRoledata->lpRoleGranteeData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pRoledata->nbrolegrantees;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_LOCATION  :
      case OT_LOCATIONWITHUSAGES:
         {
            if (!pdata1->lpLocationsData) {
               ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->locationserrcode != RES_SUCCESS)
               return pdata1->locationserrcode;
            pmemstruct = (void *) pdata1->lpLocationsData;
            if (!pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            nbelemmem=pdata1->nblocations;
            break;
         }
      case OT_ICE_MAIN :
         break;
      case OT_ICE_WEBUSER_ROLE         :
      case OT_ICE_WEBUSER_CONNECTION   :
      case OT_ICE_PROFILE_ROLE         :
      case OT_ICE_PROFILE_CONNECTION   :
		{
			struct IceObjectWithRolesAndDBData * pIceParentData;
			int iparentcount;
			switch (iobjecttype) {
				case OT_ICE_WEBUSER_ROLE         :
				case OT_ICE_WEBUSER_CONNECTION   :
					pIceParentData = pdata1->lpIceWebUsersData;
					iparentcount   = pdata1->nbicewebusers;
					break;
				case OT_ICE_PROFILE_ROLE         :
				case OT_ICE_PROFILE_CONNECTION   :
					pIceParentData = pdata1->lpIceProfilesData;
					iparentcount   = pdata1->nbiceprofiles;
					break;
			}
			if (!pIceParentData) 
				return myerror(ERR_DOMGETDATA);
			for (i=0;i<iparentcount;i++,pIceParentData=GetNextObjWithRoleAndDb(pIceParentData)) {
				if (!x_strcmp(parentstrings[0],pIceParentData->ObjectName)) {
					void * ptmp = NULL;
					switch (iobjecttype) {
						case OT_ICE_WEBUSER_ROLE:
						case OT_ICE_PROFILE_ROLE:
							ptmp = (void *)pIceParentData->lpObjectRolesData;break;
						case OT_ICE_WEBUSER_CONNECTION:
						case OT_ICE_PROFILE_CONNECTION:
							ptmp = (void *)pIceParentData->lpObjectDbData;break;
					}
					if (!ptmp) {
						ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
											bWithSystem,FALSE);
						if (ires != RES_SUCCESS)
							return ires;
					}
					switch (iobjecttype) {
						case OT_ICE_WEBUSER_ROLE:
						case OT_ICE_PROFILE_ROLE:
							if (pIceParentData->objectroleserrcode != RES_SUCCESS)
								return pIceParentData->objectroleserrcode;
							pmemstruct = (void *)pIceParentData->lpObjectRolesData;
							nbelemmem=pIceParentData->nbobjectroles;
							break;
						case OT_ICE_WEBUSER_CONNECTION:
						case OT_ICE_PROFILE_CONNECTION:
							if (pIceParentData->objectdbserrcode != RES_SUCCESS)
								return pIceParentData->objectdbserrcode;
							pmemstruct = (void *)pIceParentData->lpObjectDbData;
							nbelemmem=pIceParentData->nbobjectdbs;
							break;
					}
					goto getobj;
				}
			}
			return myerror(ERR_PARENTNOEXIST);
		}
		break;
      case OT_ICE_BUNIT_FACET_ROLE:
      case OT_ICE_BUNIT_FACET_USER:
      case OT_ICE_BUNIT_PAGE_ROLE:
      case OT_ICE_BUNIT_PAGE_USER:
		{
			struct IceBusinessUnitsData * pBusinessUnitdata;
			struct IceObjectWithRolesAndUsersData *pObjWithRolesAndUsr;
			int i2,n2;

			pBusinessUnitdata = pdata1->lpIceBusinessUnitsData;
			if (!pBusinessUnitdata) 
				return myerror(ERR_DOMGETDATA);
			for (i=0;i<pdata1->nbicebusinessunits;i++,pBusinessUnitdata=GetNextIceBusinessUnit(pBusinessUnitdata)) {
				if (!x_strcmp(parentstrings[0],pBusinessUnitdata->BusinessunitName)) {
					switch (iobjecttype) {
						case OT_ICE_BUNIT_FACET_ROLE:
						case OT_ICE_BUNIT_FACET_USER:
							pObjWithRolesAndUsr = (void *)pBusinessUnitdata->lpObjectFacetsData;
							n2                  = pBusinessUnitdata->nbfacets;
							break;
						case OT_ICE_BUNIT_PAGE_ROLE:
						case OT_ICE_BUNIT_PAGE_USER:
							pObjWithRolesAndUsr = (void *)pBusinessUnitdata->lpObjectPagesData;
							n2                  = pBusinessUnitdata->nbpages;
							break;
					}
					if (!pObjWithRolesAndUsr)
						return myerror(ERR_UPDATEDOMDATA1);
					for (i2=0;i2<n2;i2++,pObjWithRolesAndUsr=GetNextObjWithRoleAndUsr(pObjWithRolesAndUsr)) {
						if (!x_strcmp(parentstrings[1],pObjWithRolesAndUsr->ObjectName)) {
							void * ptmp = NULL;
							switch (iobjecttype) {
								case OT_ICE_BUNIT_FACET_ROLE:
								case OT_ICE_BUNIT_PAGE_ROLE:
									ptmp = (void *)pObjWithRolesAndUsr->lpObjectRolesData;break;
								case OT_ICE_BUNIT_FACET_USER:
								case OT_ICE_BUNIT_PAGE_USER:
									ptmp = (void *)pObjWithRolesAndUsr->lpObjectUsersData;break;
							}
							if (!ptmp) {
								ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
													bWithSystem,FALSE);
								if (ires != RES_SUCCESS)
									return ires;
							}
							switch (iobjecttype) {
								case OT_ICE_BUNIT_FACET_ROLE:
								case OT_ICE_BUNIT_PAGE_ROLE:
									if (pObjWithRolesAndUsr->objectroleserrcode != RES_SUCCESS)
										return pObjWithRolesAndUsr->objectroleserrcode;
									pmemstruct = (void *)pObjWithRolesAndUsr->lpObjectRolesData;
									nbelemmem=pObjWithRolesAndUsr->nbobjectroles;
									break;
								case OT_ICE_BUNIT_FACET_USER:
								case OT_ICE_BUNIT_PAGE_USER:
									if (pObjWithRolesAndUsr->objectuserserrcode != RES_SUCCESS)
										return pObjWithRolesAndUsr->objectuserserrcode;
									pmemstruct = (void *)pObjWithRolesAndUsr->lpObjectUsersData;
									nbelemmem=pObjWithRolesAndUsr->nbobjectusers;
									break;
							}
							goto getobj;
						}
					}
					return myerror(ERR_PARENTNOEXIST);
				}
			}
			return myerror(ERR_PARENTNOEXIST);
		}
		break;

      case OT_ICE_BUNIT_SEC_ROLE       :
      case OT_ICE_BUNIT_SEC_USER       :
      case OT_ICE_BUNIT_FACET          :
      case OT_ICE_BUNIT_PAGE           :
      case OT_ICE_BUNIT_LOCATION       :
		{
			struct IceBusinessUnitsData * pBusinessUnitdata;

			pBusinessUnitdata = pdata1->lpIceBusinessUnitsData;
			if (!pBusinessUnitdata) 
				return myerror(ERR_DOMGETDATA);
			for (i=0;i<pdata1->nbicebusinessunits;i++,pBusinessUnitdata=GetNextIceBusinessUnit(pBusinessUnitdata)) {
				if (!x_strcmp(parentstrings[0],pBusinessUnitdata->BusinessunitName)) {
					void * ptmp = NULL;
					switch (iobjecttype) {
						case OT_ICE_BUNIT_SEC_ROLE:
							ptmp = (void *)pBusinessUnitdata->lpObjectRolesData;break;
						case OT_ICE_BUNIT_SEC_USER:
							ptmp = (void *)pBusinessUnitdata->lpObjectUsersData;break;
						case OT_ICE_BUNIT_FACET:
							ptmp = (void *)pBusinessUnitdata->lpObjectFacetsData;break;
						case OT_ICE_BUNIT_PAGE:
							ptmp = (void *)pBusinessUnitdata->lpObjectPagesData;break;
						case OT_ICE_BUNIT_LOCATION:
							ptmp = (void *)pBusinessUnitdata->lpObjectLocationsData;break;
					}
					if (!ptmp) {
						ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
											bWithSystem,FALSE);
						if (ires != RES_SUCCESS)
							return ires;
					}
					switch (iobjecttype) {
						case OT_ICE_BUNIT_SEC_ROLE:
							if (pBusinessUnitdata->objectroleserrcode != RES_SUCCESS)
								return pBusinessUnitdata->objectroleserrcode;
							pmemstruct = (void *)pBusinessUnitdata->lpObjectRolesData;
							nbelemmem=pBusinessUnitdata->nbobjectroles;
							break;
						case OT_ICE_BUNIT_SEC_USER:
							if (pBusinessUnitdata->objectuserserrcode != RES_SUCCESS)
								return pBusinessUnitdata->objectuserserrcode;
							pmemstruct = (void *)pBusinessUnitdata->lpObjectUsersData;
							nbelemmem=pBusinessUnitdata->nbobjectusers;
							break;
						case OT_ICE_BUNIT_FACET:
							if (pBusinessUnitdata->facetserrcode != RES_SUCCESS)
								return pBusinessUnitdata->facetserrcode;
							pmemstruct = (void *)pBusinessUnitdata->lpObjectFacetsData;
							nbelemmem=pBusinessUnitdata->nbfacets;
							break;
						case OT_ICE_BUNIT_PAGE:
							if (pBusinessUnitdata->pageserrcode != RES_SUCCESS)
								return pBusinessUnitdata->pageserrcode;
							pmemstruct = (void *)pBusinessUnitdata->lpObjectPagesData;
							nbelemmem=pBusinessUnitdata->nbpages;
							break;
						case OT_ICE_BUNIT_LOCATION:
							if (pBusinessUnitdata->objectlocationserrcode != RES_SUCCESS)
								return pBusinessUnitdata->objectlocationserrcode;
							pmemstruct = (void *)pBusinessUnitdata->lpObjectLocationsData;
							nbelemmem=pBusinessUnitdata->nbobjectlocations;
							break;
					}
					goto getobj;
				}
			}
			return myerror(ERR_PARENTNOEXIST);
		}
		break;
      case OT_TABLE     :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->tableserrcode != RES_SUCCESS)
                     return pDBdata->tableserrcode;
                  pmemstruct = (void *) pDBdata->lpTableData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbtables;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_VIEW      :
      case OT_VIEWALL   :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpViewData) {
                     ires=UpdateDOMData(hnodestruct,OT_VIEW,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->viewserrcode != RES_SUCCESS)
                     return pDBdata->viewserrcode;
                  pmemstruct = (void *) pDBdata->lpViewData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbviews;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_INTEGRITY :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) 
                     return myerror(ERR_DOMGETDATA);
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpIntegrityData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->integritieserrcode != RES_SUCCESS)
                           return ptabledata->integritieserrcode;
                        pmemstruct = (void *) ptabledata->lpIntegrityData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        ptabledata->integritiesused=TRUE;

                        nbelemmem=ptabledata->nbintegrities;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_COLUMN :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) 
                     return myerror(ERR_DOMGETDATA);
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpColumnData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->columnserrcode != RES_SUCCESS)
                           return ptabledata->columnserrcode;
                        pmemstruct = (void *) ptabledata->lpColumnData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=ptabledata->nbcolumns;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_VIEWCOLUMN :
         {
            int i2;
            struct ViewData * pViewdata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpViewData) 
                     return myerror(ERR_DOMGETDATA);
                  pViewdata = pDBdata->lpViewData;
                  if (!pViewdata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbviews;
                         i2++,pViewdata=GetNextView(pViewdata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           pViewdata->ViewName,pViewdata->ViewOwner)) {
                        if (!pViewdata->lpColumnData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (pViewdata->columnserrcode != RES_SUCCESS)
                           return pViewdata->columnserrcode;
                        pmemstruct = (void *) pViewdata->lpColumnData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=pViewdata->nbcolumns;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_RULE   :
      case OT_RULEWITHPROC :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpRuleData) {
                           ires=UpdateDOMData(hnodestruct,OT_RULE,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->ruleserrcode != RES_SUCCESS)
                           return ptabledata->ruleserrcode;
                        pmemstruct = (void *) ptabledata->lpRuleData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=ptabledata->nbRules;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_RULEPROC :
         {
            int i2,i3;
            struct TableData * ptabledata;
            struct DBData    * pDBdata;
            struct RuleData  * pRuleData;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpRuleData) {
                           ires=UpdateDOMData(hnodestruct,OT_RULE,
                                              2,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->ruleserrcode != RES_SUCCESS)
                           return ptabledata->ruleserrcode;
                        pRuleData = ptabledata->lpRuleData;
                        if (!pRuleData) 
                           return myerror(ERR_UPDATEDOMDATA1);
                        for (i3=0;i3<ptabledata->nbRules;
                             i3++,pRuleData=GetNextRule(pRuleData)) {
                           if (!DOMTreeCmpTableNames(parentstrings[2],"",
                                 pRuleData->RuleName,pRuleData->RuleOwner)){
                              int resulttype,resultlevel;
                              LPUCHAR resultparents[2];
                              // Emb 2/3/95 to correct erratic gpf
                              UCHAR result0[MAXOBJECTNAME];
                              UCHAR result1[MAXOBJECTNAME];
                              resultparents[0] = result0;
                              resultparents[1] = result1;
                              // end of Emb 2/3/95 to correct erratic gpf

                              return DOMGetObject(hnodestruct,
                              /* procedurename is prefixed with it's owner */
                              /* this allows the empty string for the owner*/
                                pRuleData->RuleProcedure,"",OT_PROCEDURE,1,
                                parentstrings,bWithSystem,&resulttype,
                                &resultlevel, resultparents, lpobjectname,
                                lpresultowner, lpresultextrastring);
                           }
                        }
                        return myerror(ERR_PARENTNOEXIST);
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_S_ALARM_SELSUCCESS_USER    :
      case OT_S_ALARM_SELFAILURE_USER    :
      case OT_S_ALARM_DELSUCCESS_USER    :
      case OT_S_ALARM_DELFAILURE_USER    :
      case OT_S_ALARM_INSSUCCESS_USER    :
      case OT_S_ALARM_INSFAILURE_USER    :
      case OT_S_ALARM_UPDSUCCESS_USER    :
      case OT_S_ALARM_UPDFAILURE_USER    :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpRawSecurityData) { 
                     ires=UpdateDOMData(hnodestruct,OTLL_SECURITYALARM,1,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->rawsecuritieserrcode !=RES_SUCCESS)
                     return pDBdata->rawsecuritieserrcode;

                  if (1) { /* may be enhanced if filter at low level */
                     pmemstruct = (void *) pDBdata->lpRawSecurityData;
                     if (!pmemstruct) 
                        return myerror(ERR_UPDATEDOMDATA1);

                     nbelemmem=pDBdata->nbRawSecurities;
                     fstrncpy(lpllfiltermem,parentstrings[1],MAXOBJECTNAME);
                     lltypemem = iobjecttype;
                     goto getobj;
                  }
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!x_strcmp(parentstrings[1],ptabledata->TableName)){
                        if (!ptabledata->lpSecurityData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->ruleserrcode != RES_SUCCESS)
                           return ptabledata->ruleserrcode;
                        pmemstruct = (void *) ptabledata->lpSecurityData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=ptabledata->nbSecurities;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_INDEX                      :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) 
                     return myerror(ERR_DOMGETDATA);
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpIndexData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->indexeserrcode != RES_SUCCESS)
                           return ptabledata->indexeserrcode;
                        ptabledata->indexesused = TRUE;
                        pmemstruct = (void *) ptabledata->lpIndexData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=ptabledata->nbIndexes;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_TABLELOCATION              :
         {
            int i2;
            struct TableData * ptabledata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) 
                     return myerror(ERR_DOMGETDATA);
                  ptabledata = pDBdata->lpTableData;
                  if (!ptabledata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbtables;
                         i2++,ptabledata=GetNextTable(ptabledata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner)) {
                        if (!ptabledata->lpTableLocationData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->tablelocationserrcode != RES_SUCCESS)
                           return ptabledata->tablelocationserrcode;
                        ptabledata->tablelocationsused=TRUE;
                        pmemstruct = (void *) ptabledata->lpTableLocationData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=ptabledata->nbTableLocations;
                        {
                           int i,iresloc,irestype,ireslev;
                           UCHAR resultobjectname[MAXOBJECTNAME];
                           UCHAR resultowner     [MAXOBJECTNAME];
                           struct LocationData *pTableLocation=
                               ptabledata->lpTableLocationData;
                           for (i=0;i<nbelemmem;i++,
                                pTableLocation=pTableLocation->pnext) {
                             if (!pTableLocation)
                               return myerror(ERR_LIST);
                             iresloc=DOMGetObject(
                                   hnodestruct,
                                   pTableLocation->LocationName,
                                   "",
                                   OT_LOCATION,
                                   0,
                                   (LPUCHAR *) 0, /* parentstrings */
                                   bWithSystem,
                                   &irestype,
                                   &ireslev,
                                   (LPUCHAR *) 0, /* resultparentstrings */
                                   resultobjectname,
                                   resultowner,
                                   pTableLocation->LocationAreaStart);
                             if (iresloc!=RES_SUCCESS)
                                return iresloc;
                           }

                        }
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_PROCEDURE                  :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpProcedureData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->procedureserrcode != RES_SUCCESS)
                     return pDBdata->procedureserrcode;
                  pmemstruct = (void *) pDBdata->lpProcedureData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbProcedures;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_SEQUENCE:
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpSequenceData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->sequenceserrcode != RES_SUCCESS)
                     return pDBdata->sequenceserrcode;
                  pmemstruct = (void *) pDBdata->lpSequenceData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbSequences;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_SCHEMAUSER                 :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpSchemaUserData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->schemaserrcode != RES_SUCCESS)
                     return pDBdata->schemaserrcode;
                  pmemstruct = (void *) pDBdata->lpSchemaUserData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbSchemas;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_SYNONYM                    :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpSynonymData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->synonymserrcode != RES_SUCCESS)
                     return pDBdata->synonymserrcode;
                  pmemstruct = (void *) pDBdata->lpSynonymData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbSynonyms;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_DBEVENT                    :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpDBEventData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->dbeventserrcode != RES_SUCCESS)
                     return pDBdata->dbeventserrcode;
                  pmemstruct = (void *) pDBdata->lpDBEventData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbDBEvents;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_REPLIC_SERVER  :
         {
            struct DBData * pDBdata;
			char buf1[100];
			pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(GetMonInfoName(hnodestruct, OT_DATABASE, parentstrings, buf1,sizeof(buf1)-1),
			               pDBdata->DBName)) {
                  if (!pDBdata->lpMonReplServerData) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->monreplservererrcode != RES_SUCCESS)
                     return pDBdata->monreplservererrcode;
                  pmemstruct = (void *) pDBdata->lpMonReplServerData;
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  nbelemmem=pDBdata->nbmonreplservers;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_REPLIC_CONNECTION :
      case OT_REPLIC_CDDS       :
      case OT_REPLIC_CDDS_DETAIL:
      case OT_REPLIC_REGTABLE   :
      case OT_REPLIC_MAILUSER   :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  switch (iobjecttypemem) {
                     case OT_REPLIC_CONNECTION :
                        pmemstruct = (void *) pDBdata->lpReplicConnectionData;
                        break;
                     case OT_REPLIC_CDDS_DETAIL:
                        fstrncpy(lpllfiltermem,parentstrings[1],MAXOBJECTNAME);
                     case OT_REPLIC_CDDS       :
                        pmemstruct = (void *) pDBdata->lpRawReplicCDDSData;
                        level=1;
                        iobjecttype=OTLL_REPLIC_CDDS;
                        break;
                     case OT_REPLIC_REGTABLE   :
                        pmemstruct = (void *) pDBdata->lpReplicRegTableData;
                        break;
                     case OT_REPLIC_MAILUSER   :
                        pmemstruct = (void *) pDBdata->lpReplicMailErrData;
                        break;
                  }
                  if (!pmemstruct) {
                     ires=UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  switch (iobjecttypemem) {
                     case OT_REPLIC_CONNECTION :
                        if (pDBdata->replicconnectionserrcode != RES_SUCCESS)
                           return pDBdata->replicconnectionserrcode;
                        nbelemmem=pDBdata->nbReplicConnections;
                        pmemstruct = (void *) pDBdata->lpReplicConnectionData;
                        break;
                     case OTLL_REPLIC_CDDS :
                     case OT_REPLIC_CDDS       :
                     case OT_REPLIC_CDDS_DETAIL:
                        if (pDBdata->rawrepliccddserrcode != RES_SUCCESS)
                           return pDBdata->rawrepliccddserrcode;
                        nbelemmem=pDBdata->nbRawReplicCDDS;
                        pmemstruct = (void *) pDBdata->lpRawReplicCDDSData;
                        break;
                     case OT_REPLIC_REGTABLE   :
                        if (pDBdata->replicregtableserrcode != RES_SUCCESS)
                           return pDBdata->replicregtableserrcode;
                        nbelemmem=pDBdata->nbReplicRegTables;
                        pmemstruct = (void *) pDBdata->lpReplicRegTableData;
                        break;
                     case OT_REPLIC_MAILUSER   :
                        if (pDBdata->replicmailerrserrcode != RES_SUCCESS)
                           return pDBdata->replicmailerrserrcode;
                        nbelemmem=pDBdata->nbReplicMailErrs;
                        pmemstruct = (void *) pDBdata->lpReplicMailErrData;
                        break;
                  }
                  if (!pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_VIEWTABLE                  :
         {
            int i2;
            struct ViewData * pViewdata;
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpViewData) 
                     return myerror(ERR_DOMGETDATA);
                  pViewdata = pDBdata->lpViewData;
                  if (!pViewdata) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i2=0;i2<pDBdata->nbviews;
                         i2++,pViewdata=GetNextView(pViewdata)) {
                     if (!DOMTreeCmpTableNames(parentstrings[1],"",
                           pViewdata->ViewName,pViewdata->ViewOwner)) {
                        if (!pViewdata->lpViewTableData) {
                           ires=UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (pViewdata->viewtableserrcode != RES_SUCCESS)
                           return pViewdata->viewtableserrcode;
                        pmemstruct = (void *) pViewdata->lpViewTableData;
                        if (!pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        nbelemmem=pViewdata->nbViewTables;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_DBGRANT_ACCESY_USER   :
      case OT_DBGRANT_ACCESN_USER   :
      case OT_DBGRANT_CREPRY_USER   :
      case OT_DBGRANT_CREPRN_USER   :
      case OT_DBGRANT_CRETBY_USER   :
      case OT_DBGRANT_CRETBN_USER   :
      case OT_DBGRANT_DBADMY_USER   :
      case OT_DBGRANT_DBADMN_USER   :
      case OT_DBGRANT_LKMODY_USER   :
      case OT_DBGRANT_LKMODN_USER   :
      case OT_DBGRANT_QRYIOY_USER   :
      case OT_DBGRANT_QRYION_USER   :
      case OT_DBGRANT_QRYRWY_USER   :
      case OT_DBGRANT_QRYRWN_USER   :
      case OT_DBGRANT_UPDSCY_USER   :
      case OT_DBGRANT_UPDSCN_USER   :
      case OT_DBGRANT_SELSCY_USER   :
      case OT_DBGRANT_SELSCN_USER   :
      case OT_DBGRANT_CNCTLY_USER   :
      case OT_DBGRANT_CNCTLN_USER   :
      case OT_DBGRANT_IDLTLY_USER   :
      case OT_DBGRANT_IDLTLN_USER   :
      case OT_DBGRANT_SESPRY_USER   :
      case OT_DBGRANT_SESPRN_USER   :
      case OT_DBGRANT_TBLSTY_USER   :
      case OT_DBGRANT_TBLSTN_USER   :
      case OT_DBGRANT_QRYCPN_USER   :
      case OT_DBGRANT_QRYCPY_USER   :
      case OT_DBGRANT_QRYPGN_USER   :
      case OT_DBGRANT_QRYPGY_USER   :
      case OT_DBGRANT_QRYCON_USER   :
      case OT_DBGRANT_QRYCOY_USER   :
      case OT_DBGRANT_SEQCRN_USER   :
      case OT_DBGRANT_SEQCRY_USER   :
         {
            if (!pdata1->lpRawDBGranteeData) {
               ires=UpdateDOMData(hnodestruct,OTLL_DBGRANTEE,0,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            pmemstruct = (void *) pdata1->lpRawDBGranteeData;
            if (!pmemstruct) 
               return myerror(ERR_DOMGETDATA);
            nbelemmem=pdata1->nbRawDBGrantees;
			if (parentstrings[0])
               fstrncpy(lpllfiltermem,parentstrings[0],MAXOBJECTNAME);
			else
               fstrncpy(lpllfiltermem,"",MAXOBJECTNAME);
			// TO BE FINISHED temporary fix for install level (fix in the caller would be cleaner)

            lltypemem = iobjecttype;
            goto getobj;
         }
         break;
      case OT_S_ALARM_CO_SUCCESS_USER :
      case OT_S_ALARM_CO_FAILURE_USER :
      case OT_S_ALARM_DI_SUCCESS_USER :
      case OT_S_ALARM_DI_FAILURE_USER :
         {
            if (!pdata1->lpRawDBSecurityData) {
               ires=UpdateDOMData(hnodestruct,OTLL_DBSECALARM,0,parentstrings,
                             bWithSystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            pmemstruct = (void *) pdata1->lpRawDBSecurityData;
            if (!pmemstruct) 
               return myerror(ERR_DOMGETDATA);
            nbelemmem=pdata1->nbRawDBSecurities;
			if (parentstrings[0])
               fstrncpy(lpllfiltermem,parentstrings[0],MAXOBJECTNAME);
			else
               fstrncpy(lpllfiltermem,"",MAXOBJECTNAME);
			// TO BE FINISHED temporary fix for install level (fix in the caller would be cleaner)
            lltypemem = iobjecttype;
            goto getobj;
         }
         break;
      case OT_TABLEGRANT_SEL_USER :
      case OT_TABLEGRANT_INS_USER :
      case OT_TABLEGRANT_UPD_USER :
      case OT_TABLEGRANT_DEL_USER :
      case OT_TABLEGRANT_REF_USER :
      case OT_TABLEGRANT_CPI_USER :
      case OT_TABLEGRANT_CPF_USER :
      case OT_TABLEGRANT_ALL_USER :
      case OT_TABLEGRANT_INDEX_USER:
      case OT_TABLEGRANT_ALTER_USER:
      case OT_VIEWGRANT_SEL_USER  :
      case OT_VIEWGRANT_INS_USER  :
      case OT_VIEWGRANT_UPD_USER  :
      case OT_VIEWGRANT_DEL_USER  :
      case OT_DBEGRANT_RAISE_USER :
      case OT_DBEGRANT_REGTR_USER :
      case OT_PROCGRANT_EXEC_USER :
      case OT_SEQUGRANT_NEXT_USER :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);

            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpRawGranteeData) { 
                     ires=UpdateDOMData(hnodestruct,OTLL_GRANTEE,1,
                                        parentstrings,bWithSystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  else {
                     if (bWithSystem && !pDBdata->HasSystemRawGrantees) {
                        ires=UpdateDOMData(hnodestruct,OTLL_GRANTEE,1,
                                           parentstrings,bWithSystem,FALSE);
                        if (ires != RES_SUCCESS)
                           return ires;
                     }
                  }
                  if (pDBdata->rawgranteeserrcode !=RES_SUCCESS)
                     return pDBdata->rawgranteeserrcode;

                  if (1) { 
                     pmemstruct = (void *) pDBdata->lpRawGranteeData;
                     if (!pmemstruct) 
                        return myerror(ERR_UPDATEDOMDATA1);

                     nbelemmem=pDBdata->nbRawGrantees;
                     fstrncpy(lpllfiltermem,parentstrings[1],MAXOBJECTNAME);
                     lltypemem = iobjecttype;
                     goto getobj;
                  }
                  return myerror(ERR_PARENTNOEXIST);
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
         return RES_ERR;
         break;
      default:
         return RES_ERR;
         break;
   }
getobj:
   iObject=0;
   return DOMGetNextObject(lpobjectname, lpresultowner,lpresultextrastring);
}


int  DOMGetNextObject ( lpobjectname, lpresultowner,lpresultextrastring)
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;
LPUCHAR lpresultextrastring;
{
   UCHAR ownerforfilter[MAXOBJECTNAME];
   UCHAR buf[100];

   int iobjecttype         = iobjecttypemem; 
   LPUCHAR lpowner = (LPUCHAR) lpownermem;

   if (!lpresultowner)
      lpresultowner=ownerforfilter;

   if (lpowner && !lpowner[0])
      lpowner=(LPUCHAR)0;

   if (!lpobjectname)
      return myerror(ERR_DOMDATA);

   while (TRUE) {

      if (iObject >= nbelemmem)
         return RES_ENDOFDATA;

      if (!pmemstruct) {
         myerror(ERR_LIST);
         return RES_ENDOFDATA;
      }

      switch (iobjecttype) {
// monitoring
         case OT_MON_SERVER         :
            {
               LPSERVERDATA pdata = pmemstruct;
               * ( (LPSERVERDATAMIN) lpobjectname ) = pdata->ServerDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOCKLIST       :
            {
               LPLOCKLISTDATA pdata = pmemstruct;
               while (pdata->LockListDta.bIs4AllLockLists)  {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pdata=pdata->pnext;
               }
               * ( (LPLOCKLISTDATAMIN) lpobjectname ) = pdata->LockListDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OTLL_MON_RESOURCE       :
            {
               LPRESOURCEDATA pdata = pmemstruct;
               * ( (LPRESOURCEDATAMIN) lpobjectname ) = pdata->ResourceDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOGPROCESS     :
            {
               LPLOGPROCESSDATA pdata = pmemstruct;
               * ( (LPLOGPROCESSDATAMIN) lpobjectname ) = pdata->LogProcessDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOGDATABASE    :
            {
               LPLOGDBDATA pdata = pmemstruct;
               * ( (LPLOGDBDATAMIN) lpobjectname ) = pdata->LogDBDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_TRANSACTION:
            {
               LPLOGTRANSACTDATA pdata = pmemstruct;
               * ( (LPLOGTRANSACTDATAMIN) lpobjectname ) = pdata->LogTransactDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_SESSION        :
            {
               LPSESSIONDATA pdata = pmemstruct;
               * ( (LPSESSIONDATAMIN) lpobjectname ) = pdata->SessionDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_CONNECTED_USER:
            {
               LPICECONNUSERDATA pdata = pmemstruct;
               * ( (LPICE_CONNECTED_USERDATAMIN) lpobjectname ) = pdata->IceConnUserDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_ACTIVE_USER   :
            {
               LPICEACTIVEUSERDATA pdata = pmemstruct;
               * ( (LPICE_ACTIVE_USERDATAMIN) lpobjectname ) = pdata->IceActiveUserDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_TRANSACTION   :
            {
               LPICETRANSACTDATA pdata = pmemstruct;
               * ( (LPICE_TRANSACTIONDATAMIN) lpobjectname ) = pdata->IceTransactDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_CURSOR        :
            {
               LPICECURSORDATA pdata = pmemstruct;
               * ( (LPICE_CURSORDATAMIN) lpobjectname ) = pdata->IceCursorDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_FILEINFO      :
            {
               LPICECACHEDATA pdata = pmemstruct;
               * ( (LPICE_CACHEINFODATAMIN) lpobjectname ) = pdata->IceCacheDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_DB_CONNECTION :
            {
               LPICEDBCONNECTDATA pdata = pmemstruct;
               * ( (LPICE_DB_CONNECTIONDATAMIN) lpobjectname ) = pdata->IceDbConnectDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOCK           :
         case OT_MON_LOCKLIST_LOCK  :
         case OT_MON_RES_LOCK   :
            {
               LPLOCKDATA pdata = pmemstruct;
               * ( (LPLOCKDATAMIN) lpobjectname ) = pdata->LockDta;
               pmemstruct = pdata->pnext;
               break;
            }
// end of monitoring

		 case OT_MON_REPLIC_SERVER  :
			{
			   LPMONREPLSERVERDATA pdata=pmemstruct;
               * ( (LPREPLICSERVERDATAMIN) lpobjectname ) = pdata->MonReplServerDta;
               pmemstruct = pdata->pnext;
               break;
			}
         case OT_NODE  :
            {
               LPMONNODEDATA pdata = pmemstruct;
               * ( (LPNODEDATAMIN) lpobjectname ) = pdata->MonNodeDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_NODE_SERVERCLASS:
            {
               LPNODESERVERCLASSDATA pdata = pmemstruct;
               * ( (LPNODESVRCLASSDATAMIN) lpobjectname ) = pdata->NodeSvrClassDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_NODE_LOGINPSW:
            {
               LPNODELOGINDATA pdata = pmemstruct;
               * ( (LPNODELOGINDATAMIN) lpobjectname ) = pdata->NodeLoginDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_NODE_CONNECTIONDATA:
            {
               LPNODECONNECTDATA pdata = pmemstruct;
               * ( (LPNODECONNECTDATAMIN) lpobjectname ) = pdata->NodeConnectionDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_NODE_ATTRIBUTE:
            {
               LPNODEATTRIBUTEDATA pdata = pmemstruct;
               * ( (LPNODEATTRIBUTEDATAMIN) lpobjectname ) = pdata->NodeAttributeDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_NODE_OPENWIN  :
            {
               LPNODEWINDOWDATA pdata = pmemstruct;
               * ( (LPWINDOWDATAMIN) lpobjectname ) = pdata->WindowDta;
               pmemstruct = pdata->pnext;
               break;
            }
         case OT_DATABASE  :
            {
               struct DBData * pDBData = pmemstruct;

               while (lpowner && x_strcmp(pDBData->DBOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pDBData=pDBData->pnext;
               }
               fstrncpy(lpobjectname,pDBData->DBName,
                        sizeof(pDBData->DBName));

               if (lpresultowner) {
                  fstrncpy(lpresultowner,pDBData->DBOwner,
                           sizeof(pDBData->DBOwner));
               }
               if (lpresultextrastring)  {
                 storeint(lpresultextrastring,pDBData->DBid);
                 storeint(lpresultextrastring+STEPSMALLOBJ,pDBData->DBType);
				 x_strcpy(lpresultextrastring+2*STEPSMALLOBJ,pDBData->CDBName);
               }

               pmemstruct = pDBData->pnext;
               break;
            }
      case OT_ICE_MAIN :
        break;
      case OT_ICE_WEBUSER              :
      case OT_ICE_PROFILE              :
			{
			   struct IceObjectWithRolesAndDBData * pObjdata = pmemstruct;

			   fstrncpy(lpobjectname,pObjdata->ObjectName,
						sizeof(pObjdata->ObjectName));
   
			   pmemstruct = pObjdata->pnext;
			   break;
			}
      case OT_ICE_BUNIT                :
			{
			   struct IceBusinessUnitsData * pObjdata = pmemstruct;

			   fstrncpy(lpobjectname,pObjdata->BusinessunitName,
						sizeof(pObjdata->BusinessunitName));
   
			   pmemstruct = pObjdata->pnext;
			   break;
			}
		case OT_ICE_BUNIT_FACET          :
		case OT_ICE_BUNIT_PAGE           :
			{
				struct IceObjectWithRolesAndUsersData * pObjdata = pmemstruct;

				fstrncpy(lpobjectname,pObjdata->ObjectName,
						sizeof(pObjdata->ObjectName));
   
				pmemstruct = pObjdata->pnext;
				break;
			}

		case OT_ICE_ROLE                 :
		case OT_ICE_DBUSER               :
		case OT_ICE_DBCONNECTION         :
		case OT_ICE_WEBUSER_ROLE         :
		case OT_ICE_WEBUSER_CONNECTION   :
		case OT_ICE_PROFILE_ROLE         :
		case OT_ICE_PROFILE_CONNECTION   :
		case OT_ICE_BUNIT_SEC_ROLE       :
		case OT_ICE_BUNIT_SEC_USER       :
		case OT_ICE_BUNIT_FACET_ROLE     :
		case OT_ICE_BUNIT_FACET_USER     :
		case OT_ICE_BUNIT_PAGE_ROLE      :
		case OT_ICE_BUNIT_PAGE_USER      :
		case OT_ICE_BUNIT_LOCATION       :
		case OT_ICE_SERVER_APPLICATION   :
		case OT_ICE_SERVER_LOCATION      :
		case OT_ICE_SERVER_VARIABLE      :
			{
				struct SimpleLeafObjectData * pObjdata = pmemstruct;

				fstrncpy(lpobjectname,pObjdata->ObjectName,
						sizeof(pObjdata->ObjectName));
				switch (iobjecttype) {
					case OT_ICE_BUNIT_FACET_ROLE     :
					case OT_ICE_BUNIT_FACET_USER     :
					case OT_ICE_BUNIT_PAGE_ROLE      :
					case OT_ICE_BUNIT_PAGE_USER      :
						if (lpresultextrastring) 
							storeint(lpresultextrastring+3*STEPSMALLOBJ,pObjdata->b4);
						/* no break here , must continue with code of 2 other types below*/
					case OT_ICE_BUNIT_SEC_ROLE       :
					case OT_ICE_BUNIT_SEC_USER       :
						if (lpresultextrastring) {
							storeint(lpresultextrastring               ,pObjdata->b1);
							storeint(lpresultextrastring+  STEPSMALLOBJ,pObjdata->b2);
							storeint(lpresultextrastring+2*STEPSMALLOBJ,pObjdata->b3);
						}
						break;
				}

				pmemstruct = pObjdata->pnext;
				break;
			}

         case OT_PROFILE   :
            {
               struct ProfileData * pProfiledata = pmemstruct;

               fstrncpy(lpobjectname,pProfiledata->ProfileName,
                        sizeof(pProfiledata->ProfileName));
               
               pmemstruct = pProfiledata->pnext;
               break;
            }
         case OT_USER      :
            {
               struct UserData * puserdata = pmemstruct;

               fstrncpy(lpobjectname,puserdata->UserName,
                        sizeof(puserdata->UserName));
               
               if (lpresultextrastring) {
                  if (puserdata->bAuditAll)
                     lpresultextrastring[0]=0x01;
                  else
                     lpresultextrastring[0]=0x00;
                  lpresultextrastring[1]=0x00;
               }


               pmemstruct = puserdata->pnext;
               break;
            }
         case OT_GROUP     :
            {
               struct GroupData * pgroupdata = pmemstruct;

               fstrncpy(lpobjectname,pgroupdata->GroupName,
                        sizeof(pgroupdata->GroupName));

               pmemstruct = pgroupdata->pnext;
               break;
            }
         case OT_GROUPUSER :
            {
               struct GroupUserData * pgroupuserdata = pmemstruct;

               fstrncpy(lpobjectname,pgroupuserdata->GroupUserName,
                        sizeof(pgroupuserdata->GroupUserName));

               pmemstruct = pgroupuserdata->pnext;
               break;
            }
         case OT_ROLE      :
            {
               struct RoleData * proledata = pmemstruct;

               fstrncpy(lpobjectname,proledata->RoleName,
                        sizeof(proledata->RoleName));

               pmemstruct = proledata->pnext;
               break;
            }
         case OT_ROLEGRANT_USER :
            {
               struct RoleGranteeData * pRoleGranteedata = pmemstruct;

               fstrncpy(lpobjectname,pRoleGranteedata->RoleGranteeName,
                        sizeof(pRoleGranteedata->RoleGranteeName));

               pmemstruct = pRoleGranteedata->pnext;
               break;
            }
         case OT_LOCATION  :
         case OT_LOCATIONWITHUSAGES:
            {
               struct LocationData * plocationdata = pmemstruct;

               fstrncpy(lpobjectname,plocationdata->LocationName,
                        sizeof(plocationdata->LocationName));

               if (lpresultextrastring) {
                  fstrncpy(lpresultextrastring,plocationdata->LocationAreaStart,
                                               MAXOBJECTNAME);
                  if (iobjecttype==OT_LOCATIONWITHUSAGES) {
                     memcpy(lpresultextrastring+MAXOBJECTNAME,
                            plocationdata->LocationUsages,
                            sizeof(plocationdata->LocationUsages));
                  }
               }
               pmemstruct = plocationdata->pnext;
               break;
            }
         case OT_TABLE     :
            {
               struct TableData * pTableData = pmemstruct;

               while (lpowner && x_strcmp(pTableData->TableOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pTableData=pTableData->pnext;
               }
               fstrncpy(lpobjectname,pTableData->TableName,
                        sizeof(pTableData->TableName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pTableData->TableOwner,
                           sizeof(pTableData->TableOwner));
               }
               if (lpresultextrastring) {
                 storeint(lpresultextrastring,                pTableData->Tableid);
                 storeint(lpresultextrastring + STEPSMALLOBJ, pTableData->TableStarType);
			   }
               pmemstruct = pTableData->pnext;
               break;
            }
         case OT_VIEW      :
         case OT_VIEWALL   :
            {
               struct ViewData * pViewData = pmemstruct;

               while (lpowner && x_strcmp(pViewData->ViewOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pViewData=pViewData->pnext;
               }
               fstrncpy(lpobjectname,pViewData->ViewName,
                        sizeof(pViewData->ViewName));

               if (lpresultowner) {
                  fstrncpy(lpresultowner,pViewData->ViewOwner,
                           sizeof(pViewData->ViewOwner));
               }
               if (lpresultextrastring) {
                  storeint(lpresultextrastring + 4               , pViewData->Tableid);
                  storeint(lpresultextrastring + 4 + STEPSMALLOBJ, pViewData->ViewStarType);
                  storeint(lpresultextrastring + 4 + (STEPSMALLOBJ*2), pViewData->bSQL);
               }
               pmemstruct = pViewData->pnext;
               break;
            }
         case OT_INTEGRITY :
            {
               struct IntegrityData * pIntegrityData = pmemstruct;

               fstrncpy(lpobjectname,pIntegrityData->IntegrityName,
                        sizeof(pIntegrityData->IntegrityName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pIntegrityData->IntegrityOwner,
                           sizeof(pIntegrityData->IntegrityOwner));
               }
               if (lpresultextrastring)
                  storeint(lpresultextrastring,
                           (int)pIntegrityData->IntegrityNumber);
               pmemstruct = pIntegrityData->pnext;
               break;
            }
            break;
         case OT_VIEWCOLUMN :
         case OT_COLUMN :
            {
               struct ColumnData * pColumnData = pmemstruct;

               fstrncpy(lpobjectname,pColumnData->ColumnName,
                        sizeof(pColumnData->ColumnName));
               
               if (lpresultextrastring) {
                     storeint(lpresultextrastring,pColumnData->ColumnType);
                     storeint(lpresultextrastring+STEPSMALLOBJ,
                              pColumnData->ColumnLen);
               }
               pmemstruct = pColumnData->pnext;
               break;
            }
            break;
         case OT_RULE :
         case OT_RULEWITHPROC :
            {
               struct RuleData * pRuleData = pmemstruct;

               fstrncpy(lpobjectname,pRuleData->RuleName,
                        sizeof(pRuleData->RuleName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pRuleData->RuleOwner,
                           sizeof(pRuleData->RuleOwner));
               }
               if (lpresultextrastring) {
                  fstrncpy(lpresultextrastring,pRuleData->RuleText,
                           sizeof(pRuleData->RuleText));
                  if (iobjecttype==OT_RULEWITHPROC)
                     fstrncpy(lpresultextrastring + MAXOBJECTNAME,
                              pRuleData->RuleProcedure, MAXOBJECTNAME);
               }
               pmemstruct = pRuleData->pnext;
               break;
            }
            break;
         case OT_INDEX :
            {
               struct IndexData * pIndexData = pmemstruct;

               fstrncpy(lpobjectname,pIndexData->IndexName,MAXOBJECTNAME);
               if (lpresultowner) 
                  fstrncpy(lpresultowner,pIndexData->IndexOwner,MAXOBJECTNAME);
               if (lpresultextrastring) {
                  fstrncpy(lpresultextrastring,pIndexData->IndexStorage,
                                               23);
                  storeint(lpresultextrastring+24,pIndexData->IndexId);
               }
               pmemstruct = pIndexData->pnext;
               break;
            }
            break;
         case OT_TABLELOCATION :
            {
               struct LocationData * pTableLocationData = pmemstruct;

               fstrncpy(lpobjectname,pTableLocationData->LocationName,
                        sizeof(pTableLocationData->LocationName));

               if (lpresultextrastring) {
                  fstrncpy(lpresultextrastring,
                           pTableLocationData->LocationAreaStart,MAXOBJECTNAME);
               }

               bsystemmem = TRUE;
               pmemstruct = pTableLocationData->pnext;
               break;
            }
            break;
         case OT_S_ALARM_SELSUCCESS_USER :
         case OT_S_ALARM_SELFAILURE_USER :
         case OT_S_ALARM_DELSUCCESS_USER :
         case OT_S_ALARM_DELFAILURE_USER :
         case OT_S_ALARM_INSSUCCESS_USER :
         case OT_S_ALARM_INSFAILURE_USER :
         case OT_S_ALARM_UPDSUCCESS_USER :
         case OT_S_ALARM_UPDFAILURE_USER :
         case OT_S_ALARM_CO_SUCCESS_USER :
         case OT_S_ALARM_CO_FAILURE_USER :
         case OT_S_ALARM_DI_SUCCESS_USER :
         case OT_S_ALARM_DI_FAILURE_USER :
            {
               struct RawSecurityData * pRawSecurity = pmemstruct;
               BOOL bHasSchema;
               switch (iobjecttype) {
                  case OT_S_ALARM_CO_SUCCESS_USER :
                  case OT_S_ALARM_CO_FAILURE_USER :
                  case OT_S_ALARM_DI_SUCCESS_USER :
                  case OT_S_ALARM_DI_FAILURE_USER :
                    bHasSchema=FALSE;
                    break;
                  default:
                    bHasSchema=TRUE;
                    break;
               }

               while (
                      lltypemem != pRawSecurity->RSecurityType ||
                      (bHasSchema &&
                       DOMTreeCmpTableNames(pRawSecurity->RSecurityTable,
                                            pRawSecurity->RSecurityTableOwner,
                                            lpllfiltermem,""))  ||
                      (!bHasSchema &&
                       x_strcmp(pRawSecurity->RSecurityTable,lpllfiltermem))
                     ){
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pRawSecurity=pRawSecurity->pnext;
               }
               fstrncpy(lpobjectname,pRawSecurity->RSecurityUser,
                        sizeof(pRawSecurity->RSecurityUser));
               if (lpresultowner) {
                 fstrncpy(lpresultowner,pRawSecurity->RSecurityDBEvent,
                        MAXOBJECTNAME);
               }
               if (lpresultextrastring){ 
                  storeint(lpresultextrastring,
                           (int)pRawSecurity->RSecurityNumber);
                  fstrncpy(lpresultextrastring+STEPSMALLOBJ,
                           pRawSecurity->RSecurityName,
                           MAXOBJECTNAMENOSCHEMA);
               }
               pmemstruct = pRawSecurity->pnext;
               break;
            }
            break;
         case OT_PROCEDURE    :
            {
               struct ProcedureData * pProcedureData = pmemstruct;

               while (lpowner && x_strcmp(pProcedureData->ProcedureOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pProcedureData=pProcedureData->pnext;
               }
               fstrncpy(lpobjectname,pProcedureData->ProcedureName,
                        sizeof(pProcedureData->ProcedureName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pProcedureData->ProcedureOwner,
                           sizeof(pProcedureData->ProcedureOwner));
               }
               if (lpresultextrastring)  {
                  fstrncpy(lpresultextrastring,pProcedureData->ProcedureText,
                           sizeof(pProcedureData->ProcedureText));
               }
               pmemstruct = pProcedureData->pnext;
               break;
            }
         case OT_SEQUENCE:
            {
               struct SequenceData * pSequenceData = pmemstruct;

               while (lpowner && x_strcmp(pSequenceData->SequenceOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pSequenceData=pSequenceData->pnext;
               }
               fstrncpy(lpobjectname,pSequenceData->SequenceName,
                        sizeof(pSequenceData->SequenceName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pSequenceData->SequenceOwner,
                           sizeof(pSequenceData->SequenceOwner));
               }
               if (lpresultextrastring)  {
                  x_strcpy(lpresultextrastring,"");
               }
               pmemstruct = pSequenceData->pnext;
               break;
            }
         case OT_SCHEMAUSER :
            {
               struct SchemaUserData * pSchemaUserData = pmemstruct;

               while (lpowner && x_strcmp(pSchemaUserData->SchemaUserOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pSchemaUserData=pSchemaUserData->pnext;
               }
               fstrncpy(lpobjectname,pSchemaUserData->SchemaUserName,
                        sizeof(pSchemaUserData->SchemaUserName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pSchemaUserData->SchemaUserOwner,
                           sizeof(pSchemaUserData->SchemaUserOwner));
               }
               pmemstruct = pSchemaUserData->pnext;
               break;
            }
         case OT_SYNONYM :
            {
               struct SynonymData * pSynonymData = pmemstruct;

               while (lpowner && x_strcmp(pSynonymData->SynonymOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pSynonymData=pSynonymData->pnext;
               }
               fstrncpy(lpobjectname,pSynonymData->SynonymName,
                        sizeof(pSynonymData->SynonymName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pSynonymData->SynonymOwner,
                           sizeof(pSynonymData->SynonymOwner));
               }

               if (lpresultextrastring) 
                  fstrncpy(lpresultextrastring,pSynonymData->SynonymTable,
                           sizeof(pSynonymData->SynonymTable));

               pmemstruct = pSynonymData->pnext;
               break;
            }
         case OT_DBEVENT :
            {
               struct DBEventData * pDBEventData = pmemstruct;

               while (lpowner && x_strcmp(pDBEventData->DBEventOwner,lpowner)) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pDBEventData=pDBEventData->pnext;
               }
               fstrncpy(lpobjectname,pDBEventData->DBEventName,
                        sizeof(pDBEventData->DBEventName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pDBEventData->DBEventOwner,
                           sizeof(pDBEventData->DBEventOwner));
               }

               pmemstruct = pDBEventData->pnext;
               break;
            }
         case OT_REPLIC_CONNECTION :
            {
               struct ReplicConnectionData *pReplicConnectionData= pmemstruct;

               wsprintf(buf,"%d %s::%s",pReplicConnectionData->DBNumber,
                                        pReplicConnectionData->NodeName,
                                        pReplicConnectionData->DBName);
               fstrncpy(lpobjectname,buf, MAXOBJECTNAME);
               
               if (lpresultowner) {
                   storeint(lpresultowner,pReplicConnectionData->DBNumber);
                   if (pReplicConnectionData->ReplicVersion==REPLIC_V10 ||
                       pReplicConnectionData->ReplicVersion==REPLIC_V105) {
                      storeint(lpresultowner+STEPSMALLOBJ,pReplicConnectionData->TargetType);
                   }
               }
               if (lpresultextrastring)  {
                  switch (pReplicConnectionData->ReplicVersion) {
                     case REPLIC_V10:
                     case REPLIC_V105:
                        {
                           switch (pReplicConnectionData->TargetType) {
                              case REPLIC_FULLPEER:
                                 x_strcpy(buf, ResourceString ((UINT)IDS_FULLPEER));
                                 break;
                              case REPLIC_PROT_RO:
                                 x_strcpy(buf, ResourceString ((UINT)IDS_PROTREADONLY));
                                 break;
                              case REPLIC_UNPROT_RO:
                                 x_strcpy(buf, ResourceString ((UINT)IDS_UNPROTREADONLY));
                                 break;
                              case REPLIC_EACCESS:
                                 x_strcpy(buf, ResourceString ((UINT)IDS_GATEWAY2));
                                 break;
                              default:
                                 buf[0]='\0'; break;
                           }
                           wsprintf(lpresultextrastring,"%s. Server %d",buf,
                                     pReplicConnectionData->ServerNumber);
                        }
                        break;
                     case REPLIC_V11:
                        fstrncpy(lpresultextrastring,pReplicConnectionData->DBMSType,
                                 MAXOBJECTNAME);
                        break;
                     default:
                        myerror(ERR_REPLICTYPE);
                        break;
                  }
               }

               pmemstruct = pReplicConnectionData->pnext;
               break;
            }
         case OT_REPLIC_CDDS       :
            {
               struct RawReplicCDDSData * pRawReplicCDDSData = pmemstruct;
               int itmp = pRawReplicCDDSData->CDDSNo;
               wsprintf(lpobjectname,"%d",itmp);
               if (lpresultowner)
                  storeint(lpresultowner,itmp);
               while (pRawReplicCDDSData->CDDSNo == itmp) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     break;
                  pRawReplicCDDSData = pRawReplicCDDSData->pnext;
               }
               pmemstruct = pRawReplicCDDSData;
               iObject--;
            }
            break;
         case OT_REPLIC_CDDS_DETAIL:
            {
               struct RawReplicCDDSData * pRawReplicCDDSData = pmemstruct;

               while (TRUE) {
                  wsprintf(buf,"%d",pRawReplicCDDSData->CDDSNo);
                  if (x_strcmp(buf,lpllfiltermem) ||
                        (pRawReplicCDDSData->SourceDB == -1 &&
                         pRawReplicCDDSData->LocalDB  == -1 &&
                         pRawReplicCDDSData->TargetDB == -1
                        )
                     ) {
                     iObject++;
                     if (iObject >= nbelemmem)
                        return RES_ENDOFDATA;
                     pRawReplicCDDSData = pRawReplicCDDSData->pnext;
                  }
                  else
                     break;
               }
               wsprintf(lpobjectname,"Orig:%d Loc:%d Targ:%d",
                                      pRawReplicCDDSData->SourceDB,
                                      pRawReplicCDDSData->LocalDB,
                                      pRawReplicCDDSData->TargetDB);
               if (lpresultextrastring)  {
                  storeint(lpresultextrastring,
                                          pRawReplicCDDSData->LocalDB);
                  storeint(lpresultextrastring+STEPSMALLOBJ,
                                          pRawReplicCDDSData->SourceDB);
                  storeint(lpresultextrastring+ (2*STEPSMALLOBJ),
                                          pRawReplicCDDSData->TargetDB);

               }
               pmemstruct = pRawReplicCDDSData->pnext;
            }
            break;
         case OT_REPLIC_REGTABLE   :
            {
               struct ReplicRegTableData *pReplicRegTableData= pmemstruct;
               {
                   int iloctype;
                   LPUCHAR parents1[2];
                   int iloc;
                   char buftmp[MAXOBJECTNAME];
                   if (par0[0]) 
                      fstrncpy(buftmp,par0,MAXOBJECTNAME);
                   else
                      buftmp[0]='\0';
                   parents1[0]=buftmp;
                   iloc=DOMGetObjectLimitedInfo(hnodemem,
                                       pReplicRegTableData->TableName,
                                       pReplicRegTableData->TableOwner,
                                       OT_TABLE,
                                       1,
                                       parents1, TRUE, &iloctype,
                                       lpobjectname,lpresultowner,NULL);
                   if (iloc==RES_SUCCESS)
                      goto regtb1;

               }

               fstrncpy(lpobjectname,pReplicRegTableData->TableName,
                                                           MAXOBJECTNAME);
               if (lpresultowner) {
                  fstrncpy(lpresultowner,
                           pReplicRegTableData->TableOwner,
                           MAXOBJECTNAMENOSCHEMA);
               }
regtb1:

               if (lpresultextrastring)  {
                  x_strcpy(buf, "N N N");
                  if (pReplicRegTableData->bTablesCreated)
                     buf[0]='Y';
                  if (pReplicRegTableData->bColumnsRegistered)
                     buf[2]='Y';
                  if (pReplicRegTableData->bRulesCreated)
                     buf[4]='Y';
                  wsprintf(lpresultextrastring,"%d %s",
                           pReplicRegTableData->CDDSNo, buf);
                  storeint(lpresultextrastring+MAXOBJECTNAMENOSCHEMA,
                           pReplicRegTableData->CDDSNo);
               }
               pmemstruct = pReplicRegTableData->pnext;
            }
            break;
         case OT_REPLIC_MAILUSER   :
            {
               struct ReplicMailErrData *pReplicMailErrData= pmemstruct;
               fstrncpy(lpobjectname,pReplicMailErrData->UserName,
                                                     MAXOBJECTNAME);
               pmemstruct = pReplicMailErrData->pnext;
            }
            break;
         case OT_VIEWTABLE :
            {
               struct ViewTableData * pViewTable = pmemstruct;

               fstrncpy(lpobjectname,pViewTable->TableName,
                        sizeof(pViewTable->TableName));
               fstrncpy(lpresultowner,pViewTable->TableOwner,
                        sizeof(pViewTable->TableOwner));
               
               bsystemmem = TRUE;
               pmemstruct = pViewTable->pnext;
               break;
            }
            break;
         case OT_DBGRANT_ACCESY_USER   :
         case OT_DBGRANT_ACCESN_USER   :
         case OT_DBGRANT_CREPRY_USER   :
         case OT_DBGRANT_CREPRN_USER   :
         case OT_DBGRANT_CRETBY_USER   :
         case OT_DBGRANT_CRETBN_USER   :
         case OT_DBGRANT_DBADMY_USER   :
         case OT_DBGRANT_DBADMN_USER   :
         case OT_DBGRANT_LKMODY_USER   :
         case OT_DBGRANT_LKMODN_USER   :
         case OT_DBGRANT_QRYIOY_USER   :
         case OT_DBGRANT_QRYION_USER   :
         case OT_DBGRANT_QRYRWY_USER   :
         case OT_DBGRANT_QRYRWN_USER   :
         case OT_DBGRANT_UPDSCY_USER   :
         case OT_DBGRANT_UPDSCN_USER   :
         case OT_DBGRANT_SELSCY_USER   :
         case OT_DBGRANT_SELSCN_USER   :
         case OT_DBGRANT_CNCTLY_USER   :
         case OT_DBGRANT_CNCTLN_USER   :
         case OT_DBGRANT_IDLTLY_USER   :
         case OT_DBGRANT_IDLTLN_USER   :
         case OT_DBGRANT_SESPRY_USER   :
         case OT_DBGRANT_SESPRN_USER   :
         case OT_DBGRANT_TBLSTY_USER   :
         case OT_DBGRANT_TBLSTN_USER   :
         case OT_DBGRANT_QRYCPN_USER   :
         case OT_DBGRANT_QRYCPY_USER   :
         case OT_DBGRANT_QRYPGN_USER   :
         case OT_DBGRANT_QRYPGY_USER   :
         case OT_DBGRANT_QRYCON_USER   :
         case OT_DBGRANT_QRYCOY_USER   :
         case OT_DBGRANT_SEQCRN_USER   :
         case OT_DBGRANT_SEQCRY_USER   :
            {
               struct RawDBGranteeData * pRawDBGrantee = pmemstruct;

               while (lltypemem != pRawDBGrantee->GrantType ||
                      x_strcmp(pRawDBGrantee->Database,lpllfiltermem) ) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pRawDBGrantee=pRawDBGrantee->pnext;
               }
               fstrncpy(lpobjectname,pRawDBGrantee->Grantee,MAXOBJECTNAME);

               if (lpresultextrastring) {
                 if (iobjecttype==OT_DBGRANT_QRYIOY_USER ||
                     iobjecttype==OT_DBGRANT_QRYRWY_USER ||
                     iobjecttype==OT_DBGRANT_CNCTLY_USER ||
                     iobjecttype==OT_DBGRANT_IDLTLY_USER ||
                     iobjecttype==OT_DBGRANT_SESPRY_USER ||
                     iobjecttype==OT_DBGRANT_QRYCPY_USER ||
                     iobjecttype==OT_DBGRANT_QRYPGY_USER ||
                     iobjecttype==OT_DBGRANT_QRYCOY_USER 
                    )
                    wsprintf(lpresultextrastring,"%d",pRawDBGrantee->GrantExtraInt);
                 else 
                    lpresultextrastring[0]='\0';
               }
               pmemstruct = pRawDBGrantee->pnext;
               break;
            }
            break;
         case OT_TABLEGRANT_SEL_USER :
         case OT_TABLEGRANT_INS_USER :
         case OT_TABLEGRANT_UPD_USER :
         case OT_TABLEGRANT_DEL_USER :
         case OT_TABLEGRANT_REF_USER :
         case OT_TABLEGRANT_CPI_USER :
         case OT_TABLEGRANT_CPF_USER :
         case OT_TABLEGRANT_ALL_USER :
         case OT_TABLEGRANT_INDEX_USER:
         case OT_TABLEGRANT_ALTER_USER:
         case OT_VIEWGRANT_SEL_USER  :
         case OT_VIEWGRANT_INS_USER  :
         case OT_VIEWGRANT_UPD_USER  :
         case OT_VIEWGRANT_DEL_USER  :
         case OT_DBEGRANT_RAISE_USER :
         case OT_DBEGRANT_REGTR_USER :
         case OT_PROCGRANT_EXEC_USER :
         case OT_SEQUGRANT_NEXT_USER :
            {
               struct RawGranteeData * pRawGrantee = pmemstruct;
               int localobjtype;
               switch (iobjecttype) {
                  case OT_VIEWGRANT_SEL_USER  :
                  case OT_VIEWGRANT_INS_USER  :
                  case OT_VIEWGRANT_UPD_USER  :
                  case OT_VIEWGRANT_DEL_USER  :
                     localobjtype=OT_VIEW;
                     break;
                  case OT_DBEGRANT_RAISE_USER :
                  case OT_DBEGRANT_REGTR_USER :
                     localobjtype=OT_DBEVENT;
                     break;
                  case OT_PROCGRANT_EXEC_USER :
                     localobjtype=OT_PROCEDURE;
                     break;
                  case OT_SEQUGRANT_NEXT_USER :
                     localobjtype=OT_SEQUENCE;
                     break;
                  default:
                     localobjtype=OT_TABLE;
                     break;
               }
               while (lltypemem    != pRawGrantee->GrantType         ||
                      localobjtype != pRawGrantee->GranteeObjectType ||
                      DOMTreeCmpTableNames(pRawGrantee->GranteeObject,
                                           pRawGrantee->GranteeObjectOwner,
                                           lpllfiltermem,"")
                     ) {
                  iObject++;
                  if (iObject >= nbelemmem)
                     return RES_ENDOFDATA;
                  pRawGrantee=pRawGrantee->pnext;
               }
               fstrncpy(lpobjectname,pRawGrantee->Grantee,MAXOBJECTNAME);

               pmemstruct = pRawGrantee->pnext;
               break;
            }
            break;
         default:
            return RES_ERR;
      }
      iObject++;
      if (bsystemmem ||
          !IsSystemObject(iobjecttype, lpobjectname, lpresultowner))
         break;
   }
   return RES_SUCCESS;

}

static struct ResultReverseInfos *pRevInfos = (struct ResultReverseInfos *)0;
static struct ResultReverseInfos *pCurRevInfos;
static int    nbRevInfos;
static int    RevInfoType;
static int    RevInfobSystem;
static int    iRevObject;
static UCHAR  DBFiltermem[MAXOBJECTNAME];
static UCHAR  OtherFiltermem[MAXOBJECTNAME];


static struct ResultReverseInfos *GetNextRevInfo(pCurInf)
struct ResultReverseInfos *pCurInf;
{
   if (!pCurInf) {
      myerror(ERR_LIST);
      return pCurInf;
   }
   if (! pCurInf->pnext) {
      pCurInf->pnext=ESL_AllocMem(sizeof(struct ResultReverseInfos));
      if (!pCurInf->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pCurInf->pnext;
}

BOOL freeRevInfoData()
{
   struct ResultReverseInfos *ptemp;

   while(pRevInfos) {
      ptemp=pRevInfos->pnext;
      ESL_FreeMem(pRevInfos);  
      pRevInfos = ptemp;
   }
   return TRUE;
}

int  DOMGetFirstRelObject(hnodestruct, iobjecttype, level, parentstrings,
                          bwithsystem, lpDBfilter, lpobjectfilter,
                          resultparentstrings, lpresultobjectname,
                          lpresultowner, lpresultextradata)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;      
BOOL    bwithsystem;
LPUCHAR lpDBfilter;
LPUCHAR lpobjectfilter;
LPUCHAR *resultparentstrings;
LPUCHAR lpresultobjectname;
LPUCHAR lpresultowner;      
LPUCHAR lpresultextradata;

{
   int  ires, ires1, ires2;
   int nbobj;

   UCHAR buf[MAXOBJECTNAME];  
   UCHAR bufparent[MAXOBJECTNAME];  
   UCHAR bufowner[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   UCHAR resultobjname[MAXOBJECTNAME];  
   UCHAR resultobjowner[MAXOBJECTNAME];  
   UCHAR resultextradata[EXTRADATASIZE];
   UCHAR bufobjwithowner[MAXOBJECTNAME];

   LPUCHAR aparents[MAXPLEVEL];
   LPUCHAR aparents2[MAXPLEVEL];
   struct ResultReverseInfos *pLocalCurInfos;
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   if (lpDBfilter && !lpDBfilter[0])
      lpDBfilter=(LPUCHAR)0;

   if (lpobjectfilter && !lpobjectfilter[0])
      lpobjectfilter=(LPUCHAR)0;

   if (!pRevInfos) {
      pRevInfos= (struct ResultReverseInfos *)
                  ESL_AllocMem(sizeof(struct ResultReverseInfos));
   }

   nbobj=0;
   pLocalCurInfos = pRevInfos;

   switch (iobjecttype) {

      case OT_USER :
         {
               return myerror(ERR_GW);
         }
         break;
      case OTR_GRANTEE_ROLE :
         {
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_ROLE,0,aparents,
               bwithsystem, lpobjectfilter,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[0]=pLocalCurInfos->ObjectName;
               ires1=DOMGetFirstObject(hnodestruct, OT_ROLEGRANT_USER, 1, aparents,
                       bwithsystem, lpobjectfilter, buf, bufowner, extradata);
               while (ires1==RES_SUCCESS) {
                  if (!x_strcmp(parentstrings[0],buf)) {
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                     break; /* if removed, needs to change parms of */
                            /* DOMGetFirst.. DOMGetNExt             */
                  }
                  ires1 = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                  return ires1;
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_USERGROUP :
         {
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_GROUP,0,aparents,
               bwithsystem, lpobjectfilter,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[0]=pLocalCurInfos->ObjectName;
               ires1=DOMGetFirstObject(hnodestruct, OT_GROUPUSER, 1, aparents,
                       bwithsystem, lpobjectfilter, buf, bufowner, extradata);
               while (ires1==RES_SUCCESS) {
                  if (!x_strcmp(parentstrings[0],buf)) {
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                     break; /* if removed, needs to change parms of */
                            /* DOMGetFirst.. DOMGetNExt             */
                  }
                  ires1 = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                  return ires1;
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_TABLESYNONYM:
      case OTR_VIEWSYNONYM:
      case OTR_INDEXSYNONYM:
         {
            int iloc;
            if (level !=2 && !(iobjecttype==OTR_INDEXSYNONYM && level==3))
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_SYNONYM,1,parentstrings,
               bwithsystem, lpobjectfilter,resultobjname,
               resultobjowner,resultextradata);

            while (ires==RES_SUCCESS) {
               BOOL bFound=FALSE;
               if (iobjecttype==OTR_VIEWSYNONYM)
                  bFound=x_strcmp(parentstrings[level-1],resultextradata)?FALSE:TRUE;
               else
                  bFound=DOMTreeCmpTableNames(parentstrings[level-1],"",
                             resultextradata,"")?FALSE:TRUE;
                           /* resultextradata contains the owner as prefix*/
               if (bFound) {
                  fstrncpy(pLocalCurInfos->ObjectName,resultobjname,
                           MAXOBJECTNAME);
                  fstrncpy(pLocalCurInfos->ObjectOwner,resultobjowner,
                           MAXOBJECTNAME);
                  memcpy(pLocalCurInfos->ExtraData,resultextradata,
                           EXTRADATASIZE);
                  for (iloc=0;iloc<level;iloc++) {
                    fstrncpy(pLocalCurInfos->Parents[iloc],
                             parentstrings[iloc],MAXOBJECTNAME);
                  }
                  pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                  nbobj++;
               }
               ires = DOMGetNextObject(resultobjname,
                       resultobjowner,resultextradata);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_TABLEVIEW:
         {
            if (level !=2)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_VIEW,1,parentstrings,
               bwithsystem, lpobjectfilter,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[0] = parentstrings[0];
               aparents[1] = StringWithOwner(pLocalCurInfos->ObjectName,
                                             pLocalCurInfos->ObjectOwner,
                                             bufobjwithowner);
               ires1=DOMGetFirstObject(hnodestruct, OT_VIEWTABLE, 2, aparents,
                       bwithsystem, lpobjectfilter, buf, bufowner, extradata);
               while (ires1==RES_SUCCESS) {
                  if(!DOMTreeCmpTableNames(parentstrings[1],"",buf,bufowner)){
                     fstrncpy(pLocalCurInfos->Parents[0],parentstrings[0],
                           MAXOBJECTNAME);
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                     break; /* if removed, needs to change parms of */
                            /* DOMGetFirst.. DOMGetNExt and change  */
                            /* aparents[1]                          */
                  }
                  ires1 = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                  return ires1;
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_PROC_RULE:
         {
            if (level !=2)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_TABLE,1,parentstrings,
               bwithsystem, lpobjectfilter,buf,bufowner,extradata);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[0] = parentstrings[0];
               aparents[1] = StringWithOwner(buf,bufowner,bufobjwithowner);
               ires1=DOMGetFirstObject(hnodestruct, OT_RULEWITHPROC, 2,
                      aparents, bwithsystem, lpobjectfilter,
                      resultobjname,resultobjowner,resultextradata);
               while (ires1==RES_SUCCESS) {
                  if(!DOMTreeCmpTableNames(parentstrings[1],"",
                                           resultextradata+MAXOBJECTNAME,"")){
                     fstrncpy(pLocalCurInfos->ObjectName,resultobjname,
                           MAXOBJECTNAME);
                     fstrncpy(pLocalCurInfos->ObjectOwner,resultobjowner,
                           MAXOBJECTNAME);
                     memcpy(pLocalCurInfos->ExtraData,resultextradata,
                           EXTRADATASIZE);
                     fstrncpy(pLocalCurInfos->Parents[0],parentstrings[0],
                           MAXOBJECTNAME);
                     fstrncpy(pLocalCurInfos->Parents[1],aparents[1],MAXOBJECTNAME);
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                  }
                  ires1 = DOMGetNextObject(resultobjname,resultobjowner,
                                           resultextradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                  return ires1;
               ires = DOMGetNextObject(buf,bufowner,extradata);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_REPLIC_TABLE_CDDS:
         {
            if (level !=2)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_REPLIC_REGTABLE,1,
                          parentstrings,bwithsystem, lpobjectfilter,buf,
                          bufowner,extradata);

            while (ires==RES_SUCCESS) {
               if(!DOMTreeCmpTableNames(parentstrings[1],"",buf,bufowner)){
                 int itmp = getint(extradata+MAXOBJECTNAMENOSCHEMA);
                 wsprintf(pLocalCurInfos->ObjectName,"%d",itmp);
                 storeint(pLocalCurInfos->ObjectOwner,itmp);
                 fstrncpy(pLocalCurInfos->Parents[0],parentstrings[0],
                       MAXOBJECTNAME);
                 pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                 nbobj++;
               }
               ires = DOMGetNextObject(buf,bufowner,extradata);
            }
         }
         break;
      case OTR_REPLIC_CDDS_TABLE:
         {
            int icdds,iret;
            if (level !=2)
               return myerror(ERR_LEVELNUMBER);

            iret=sscanf(parentstrings[1],"%d",&icdds);
            if (!iret)
               break;
            ires=DOMGetFirstObject ( hnodestruct, OT_REPLIC_REGTABLE,1,
                          parentstrings,bwithsystem, lpobjectfilter,buf,
                          bufowner,extradata);

            while (ires==RES_SUCCESS) {
               if (getint(extradata+MAXOBJECTNAMENOSCHEMA)==icdds) {
                    fstrncpy(pLocalCurInfos->ObjectName,buf,MAXOBJECTNAME);
                    fstrncpy(pLocalCurInfos->ObjectOwner,bufowner,MAXOBJECTNAME);
                    fstrncpy(pLocalCurInfos->Parents[0],parentstrings[0],
                          MAXOBJECTNAME);
                    pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                    nbobj++;
               }
               ires = DOMGetNextObject(buf,bufowner,extradata);
            }
         }
         break;
      case OTR_USERSCHEMA :
         {
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE, 0, aparents,
               bwithsystem, lpDBfilter,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               int dbtype=getint(pLocalCurInfos->ExtraData+STEPSMALLOBJ);
               if (dbtype!=DBTYPE_DISTRIBUTED) {
                  push4domg();
                  aparents[0]=pLocalCurInfos->ObjectName;
                  ires1=DOMGetFirstObject(hnodestruct,OT_SCHEMAUSER,1,aparents,
                          bwithsystem, lpobjectfilter, buf, bufowner, extradata);
                  while (ires1==RES_SUCCESS) {
                     if (!x_strcmp(parentstrings[0],buf)) {
                        pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                        nbobj++;
                        break; /* if removed, needs to change parms of */
                               /* DOMGetFirst.. DOMGetNExt             */
                     }
                     ires1 = DOMGetNextObject(buf, bufowner, extradata);
                  }
                  pop4domg();
                  if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                     return ires1;
               }
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
       case OTR_CDB :
         {
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE, 0, aparents,
               bwithsystem, NULL,buf,bufowner,extradata);

            while (ires==RES_SUCCESS) {
				if (!x_strcmp(parentstrings[0],buf)) {
					x_strcpy(buf,extradata+2*STEPSMALLOBJ);
					break;
				}
                ires = DOMGetNextObject(buf, bufowner, extradata);
			}
			if (ires!=RES_SUCCESS)
				return ires;

   
            ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE, 0, aparents,
               bwithsystem, NULL,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               if (!x_strcmp(pLocalCurInfos->ObjectName,buf)) {
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                     break;
			   }
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_SUCCESS)
               return ires;
         }
         break;
       case OTR_LOCATIONTABLE :
         {
            int i1,i2,i3;
            struct DBData       * pDBdata;
            struct TableData    * ptabledata;
            struct LocationData * pTableLocation;

            if (!pdata1->lpDBData)  {
               ires=UpdateDOMData(hnodestruct,OT_DATABASE,0,aparents,
                                  bwithsystem,FALSE);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            pDBdata = pdata1->lpDBData;
            if (!pDBdata)  
               return myerror(ERR_DOMGETDATA);

            for (i1=0;i1<pdata1->nbdb;i1++,pDBdata=GetNextDB(pDBdata)) {
               if (!pDBdata->lpTableData) {
                  aparents[0]=pDBdata->DBName;
                  ires=UpdateDOMData(hnodestruct,OT_TABLE,1,aparents,
                                bwithsystem,FALSE);
                  if (ires != RES_SUCCESS)
                     return ires;
               }
               ptabledata = pDBdata->lpTableData;
               if (!ptabledata) 
                  return myerror(ERR_UPDATEDOMDATA1);
               for (i2=0;i2<pDBdata->nbtables;i2++,
                      ptabledata=GetNextTable(ptabledata)) {
                  if (!ptabledata->lpTableLocationData) {
                     aparents[0] = pDBdata->DBName;
                     aparents[1] = StringWithOwner(ptabledata->TableName,
                                                   ptabledata->TableOwner,
                                                   bufobjwithowner);
                     ires=UpdateDOMData(hnodestruct,OT_TABLELOCATION,2,
                                        aparents,bwithsystem,FALSE);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  pTableLocation= ptabledata->lpTableLocationData;
                  if (!pTableLocation) 
                     return myerror(ERR_UPDATEDOMDATA1);
                  for (i3=0;i3<ptabledata->nbTableLocations;i3++,
                      pTableLocation=GetNextLocation(pTableLocation)){
                     if (!x_strcmp(parentstrings[0],
                                 pTableLocation->LocationName)) {
                        fstrncpy(pLocalCurInfos->ObjectName,
                                 ptabledata->TableName,MAXOBJECTNAME);
                        fstrncpy(pLocalCurInfos->ObjectOwner,
                                 ptabledata->TableOwner,MAXOBJECTNAME);
                        fstrncpy(pLocalCurInfos->Parents[0],pDBdata->DBName,
                                 MAXOBJECTNAME);
                        pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                        nbobj++;
                        break; /* if removed, needs to change parms of */
                               /* DOMGetFirst.. DOMGetNExt             */
                     }
                  }
               }
            }
         }
         break;
      case OTR_ALARM_SELSUCCESS_TABLE : /*same code as for OTR_LOCATIONTABLE*/
      case OTR_ALARM_SELFAILURE_TABLE :
      case OTR_ALARM_DELSUCCESS_TABLE :
      case OTR_ALARM_DELFAILURE_TABLE :
      case OTR_ALARM_INSSUCCESS_TABLE :
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :
      case OTR_GRANTEE_SEL_TABLE      :
      case OTR_GRANTEE_INS_TABLE      :
      case OTR_GRANTEE_UPD_TABLE      :
      case OTR_GRANTEE_DEL_TABLE      :
      case OTR_GRANTEE_REF_TABLE      :
      case OTR_GRANTEE_CPI_TABLE      :
      case OTR_GRANTEE_CPF_TABLE      :
      case OTR_GRANTEE_ALL_TABLE      :
      case OTR_GRANTEE_SEL_VIEW       :
      case OTR_GRANTEE_INS_VIEW       :
      case OTR_GRANTEE_UPD_VIEW       :
      case OTR_GRANTEE_DEL_VIEW       :
      case OTR_GRANTEE_EXEC_PROC      :
      case OTR_GRANTEE_NEXT_SEQU      :
      case OTR_GRANTEE_RAISE_DBEVENT  :
      case OTR_GRANTEE_REGTR_DBEVENT  :
         {
            int SecType, ResObjType;
            BOOL bIntInExtradata=FALSE;
            ResObjType=OT_TABLE;
            switch (iobjecttype) {
               case OTR_LOCATIONTABLE :
                  SecType = OT_TABLELOCATION           ; break;
               case OTR_ALARM_SELSUCCESS_TABLE :
                  SecType = OT_S_ALARM_SELSUCCESS_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_SELFAILURE_TABLE :
                  SecType = OT_S_ALARM_SELFAILURE_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_DELSUCCESS_TABLE :
                  SecType = OT_S_ALARM_DELSUCCESS_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_DELFAILURE_TABLE :
                  SecType = OT_S_ALARM_DELFAILURE_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_INSSUCCESS_TABLE :
                  SecType = OT_S_ALARM_INSSUCCESS_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_INSFAILURE_TABLE :
                  SecType = OT_S_ALARM_INSFAILURE_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_UPDSUCCESS_TABLE :
                  SecType = OT_S_ALARM_UPDSUCCESS_USER;bIntInExtradata=TRUE;break;
               case OTR_ALARM_UPDFAILURE_TABLE :
                  SecType = OT_S_ALARM_UPDFAILURE_USER;bIntInExtradata=TRUE;break;
               case OTR_GRANTEE_SEL_TABLE      :
                  SecType = OT_TABLEGRANT_SEL_USER; break;
               case OTR_GRANTEE_INS_TABLE      :
                  SecType = OT_TABLEGRANT_INS_USER; break;
               case OTR_GRANTEE_UPD_TABLE      :
                  SecType = OT_TABLEGRANT_UPD_USER; break;
               case OTR_GRANTEE_DEL_TABLE      :
                  SecType = OT_TABLEGRANT_DEL_USER; break;
               case OTR_GRANTEE_REF_TABLE      :
                  SecType = OT_TABLEGRANT_REF_USER; break;
               case OTR_GRANTEE_CPI_TABLE      :
                  SecType = OT_TABLEGRANT_CPI_USER; break;
               case OTR_GRANTEE_CPF_TABLE      :
                  SecType = OT_TABLEGRANT_CPF_USER; break;
               case OTR_GRANTEE_ALL_TABLE      :
                  SecType = OT_TABLEGRANT_ALL_USER; break;
               case OTR_GRANTEE_SEL_VIEW       :
                  SecType = OT_VIEWGRANT_SEL_USER; ResObjType=OT_VIEW; break;
               case OTR_GRANTEE_INS_VIEW       :
                  SecType = OT_VIEWGRANT_INS_USER; ResObjType=OT_VIEW; break;
               case OTR_GRANTEE_UPD_VIEW       :
                  SecType = OT_VIEWGRANT_UPD_USER; ResObjType=OT_VIEW; break;
               case OTR_GRANTEE_DEL_VIEW       :
                  SecType = OT_VIEWGRANT_DEL_USER; ResObjType=OT_VIEW; break;
               case OTR_GRANTEE_EXEC_PROC      :
                  SecType = OT_PROCGRANT_EXEC_USER;ResObjType=OT_PROCEDURE;break;
               case OTR_GRANTEE_NEXT_SEQU      :
                  SecType = OT_SEQUGRANT_NEXT_USER;ResObjType=OT_SEQUENCE;break;
               case OTR_GRANTEE_RAISE_DBEVENT  :
                  SecType=OT_DBEGRANT_RAISE_USER; ResObjType=OT_DBEVENT;break;
               case OTR_GRANTEE_REGTR_DBEVENT  :
                  SecType=OT_DBEGRANT_REGTR_USER; ResObjType=OT_DBEVENT;break;
               default:
                  return RES_ERR;
            }
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE, 0, aparents,
               bwithsystem, lpDBfilter,bufparent,
               resultobjowner,resultextradata);

            while (ires==RES_SUCCESS) {
               int dbtype=getint(resultextradata+STEPSMALLOBJ);
               if (dbtype!=DBTYPE_DISTRIBUTED) {
                  push4domg();
                  aparents[0]=bufparent;
                  ires1=DOMGetFirstObject(hnodestruct,ResObjType,1,aparents,
                      bwithsystem, lpobjectfilter, resultobjname,
                      resultobjowner,resultextradata);
                  while (ires1==RES_SUCCESS) {
                     push4domg();
                     aparents2[0] = bufparent;
                     if (CanObjectHaveSchema(ResObjType)) {
                        aparents2[1] = StringWithOwner(resultobjname,
                                                       resultobjowner,
                                                       bufobjwithowner);
                     }
                     else 
                        aparents2[1] = resultobjname;
                     ires2=DOMGetFirstObject(hnodestruct,SecType,2,
                         aparents2, bwithsystem, lpobjectfilter,
                         buf,bufowner, extradata);
                     while (ires2==RES_SUCCESS) {
                        if (!x_strcmp(parentstrings[0],buf)) {
                           fstrncpy(pLocalCurInfos->ObjectName,resultobjname,
                              MAXOBJECTNAME);
                           fstrncpy(pLocalCurInfos->ObjectOwner,resultobjowner,
                              MAXOBJECTNAME);
                           memcpy(pLocalCurInfos->ExtraData,resultextradata,
                              EXTRADATASIZE);
                           fstrncpy(pLocalCurInfos->Parents[0],bufparent,
                                 MAXOBJECTNAME);
                           if (bIntInExtradata)
                              storeint(pLocalCurInfos->ExtraData,getint(extradata));
                           pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                           nbobj++;
                          /* break;  may be discussed*/
                        }
                        ires2 = DOMGetNextObject(buf, bufowner, extradata);
                     }
                     pop4domg();
                     if (ires2!=RES_ENDOFDATA && ires2 !=RES_SUCCESS) {
                        ires1=ires2;
                        break;
                     }
                     ires1 = DOMGetNextObject( resultobjname,
                          resultobjowner,resultextradata);
                  }
                  pop4domg();
                  if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                     return ires1;
               }
               ires = DOMGetNextObject(bufparent,
                       resultobjowner,resultextradata);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      case OTR_DBGRANT_ACCESY_DB :
      case OTR_DBGRANT_ACCESN_DB :
      case OTR_DBGRANT_CREPRY_DB :
      case OTR_DBGRANT_CREPRN_DB :
      case OTR_DBGRANT_CRETBY_DB :
      case OTR_DBGRANT_CRETBN_DB :
      case OTR_DBGRANT_DBADMY_DB :
      case OTR_DBGRANT_DBADMN_DB :
      case OTR_DBGRANT_LKMODY_DB :
      case OTR_DBGRANT_LKMODN_DB :
      case OTR_DBGRANT_QRYIOY_DB :
      case OTR_DBGRANT_QRYION_DB :
      case OTR_DBGRANT_QRYRWY_DB :
      case OTR_DBGRANT_QRYRWN_DB :
      case OTR_DBGRANT_UPDSCY_DB :
      case OTR_DBGRANT_UPDSCN_DB :
      case OTR_DBGRANT_SELSCY_DB : 
      case OTR_DBGRANT_SELSCN_DB : 
      case OTR_DBGRANT_CNCTLY_DB : 
      case OTR_DBGRANT_CNCTLN_DB : 
      case OTR_DBGRANT_IDLTLY_DB : 
      case OTR_DBGRANT_IDLTLN_DB : 
      case OTR_DBGRANT_SESPRY_DB : 
      case OTR_DBGRANT_SESPRN_DB : 
      case OTR_DBGRANT_TBLSTY_DB : 
      case OTR_DBGRANT_TBLSTN_DB : 
      case OTR_DBGRANT_QRYCPN_DB :
      case OTR_DBGRANT_QRYCPY_DB :
      case OTR_DBGRANT_QRYPGN_DB :
      case OTR_DBGRANT_QRYPGY_DB :
      case OTR_DBGRANT_QRYCON_DB :
      case OTR_DBGRANT_QRYCOY_DB :
      case OTR_DBGRANT_SEQCRN_DB :
      case OTR_DBGRANT_SEQCRY_DB :
         {
            int SecType;
            switch (iobjecttype) {
               case OTR_DBGRANT_ACCESY_DB :
                  SecType = OT_DBGRANT_ACCESY_USER ; break;
               case OTR_DBGRANT_ACCESN_DB :
                  SecType = OT_DBGRANT_ACCESN_USER ; break;
               case OTR_DBGRANT_CREPRY_DB :
                  SecType = OT_DBGRANT_CREPRY_USER ; break;
               case OTR_DBGRANT_CREPRN_DB :
                  SecType = OT_DBGRANT_CREPRN_USER ; break;
               case OTR_DBGRANT_CRETBY_DB :
                  SecType = OT_DBGRANT_CRETBY_USER ; break;
               case OTR_DBGRANT_CRETBN_DB :
                  SecType = OT_DBGRANT_CRETBN_USER ; break;
               case OTR_DBGRANT_DBADMY_DB :
                  SecType = OT_DBGRANT_DBADMY_USER ; break;
               case OTR_DBGRANT_DBADMN_DB :
                  SecType = OT_DBGRANT_DBADMN_USER ; break;
               case OTR_DBGRANT_LKMODY_DB :
                  SecType = OT_DBGRANT_LKMODY_USER ; break;
               case OTR_DBGRANT_LKMODN_DB :
                  SecType = OT_DBGRANT_LKMODN_USER ; break;
               case OTR_DBGRANT_QRYIOY_DB :
                  SecType = OT_DBGRANT_QRYIOY_USER ; break;
               case OTR_DBGRANT_QRYION_DB :
                  SecType = OT_DBGRANT_QRYION_USER ; break;
               case OTR_DBGRANT_QRYRWY_DB :
                  SecType = OT_DBGRANT_QRYRWY_USER ; break;
               case OTR_DBGRANT_QRYRWN_DB :
                  SecType = OT_DBGRANT_QRYRWN_USER ; break;
               case OTR_DBGRANT_UPDSCY_DB :
                  SecType = OT_DBGRANT_UPDSCY_USER ; break;
               case OTR_DBGRANT_UPDSCN_DB :
                  SecType = OT_DBGRANT_UPDSCN_USER ; break;
               case OTR_DBGRANT_SELSCY_DB: 
                  SecType = OT_DBGRANT_SELSCY_USER ; break; 
               case OTR_DBGRANT_SELSCN_DB: 
                  SecType = OT_DBGRANT_SELSCN_USER ; break; 
               case OTR_DBGRANT_CNCTLY_DB: 
                  SecType = OT_DBGRANT_CNCTLY_USER ; break; 
               case OTR_DBGRANT_CNCTLN_DB: 
                  SecType = OT_DBGRANT_CNCTLN_USER ; break; 
               case OTR_DBGRANT_IDLTLY_DB: 
                  SecType = OT_DBGRANT_IDLTLY_USER ; break; 
               case OTR_DBGRANT_IDLTLN_DB: 
                  SecType = OT_DBGRANT_IDLTLN_USER ; break; 
               case OTR_DBGRANT_SESPRY_DB: 
                  SecType = OT_DBGRANT_SESPRY_USER ; break; 
               case OTR_DBGRANT_SESPRN_DB: 
                  SecType = OT_DBGRANT_SESPRN_USER ; break; 
               case OTR_DBGRANT_TBLSTY_DB: 
                  SecType = OT_DBGRANT_TBLSTY_USER ; break; 
               case OTR_DBGRANT_TBLSTN_DB: 
                  SecType = OT_DBGRANT_TBLSTN_USER ; break; 
               case OTR_DBGRANT_QRYCPN_DB:
                  SecType = OT_DBGRANT_QRYCPN_USER ; break; 
               case OTR_DBGRANT_QRYCPY_DB:
                  SecType = OT_DBGRANT_QRYCPY_USER ; break; 
               case OTR_DBGRANT_QRYPGN_DB:
                  SecType = OT_DBGRANT_QRYPGN_USER ; break; 
               case OTR_DBGRANT_QRYPGY_DB:
                  SecType = OT_DBGRANT_QRYPGY_USER ; break; 
               case OTR_DBGRANT_QRYCON_DB:
                  SecType = OT_DBGRANT_QRYCON_USER ; break; 
               case OTR_DBGRANT_QRYCOY_DB:
                  SecType = OT_DBGRANT_QRYCOY_USER ; break; 
               case OTR_DBGRANT_SEQCRN_DB:
                  SecType = OT_DBGRANT_SEQCRN_USER ; break; 
               case OTR_DBGRANT_SEQCRY_DB:
                  SecType = OT_DBGRANT_SEQCRY_USER ; break; 
               default:
                  return RES_ERR;
            }
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);

            ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE, 0, aparents,
               bwithsystem, lpDBfilter,pLocalCurInfos->ObjectName,
               pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[0]=pLocalCurInfos->ObjectName;
               ires1=DOMGetFirstObject(hnodestruct,SecType,1,aparents,
                   bwithsystem, lpobjectfilter,buf,bufowner, extradata);
               while (ires1==RES_SUCCESS) {
                  if (!x_strcmp(parentstrings[0],buf)) {
                     pLocalCurInfos=GetNextRevInfo(pLocalCurInfos);
                     nbobj++;
                     break; /* if removed, needs to change parms of */
                            /* DOMGetFirst.. DOMGetNExt             */
                  }
                  ires1 = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA && ires1 !=RES_SUCCESS)
                  return ires1;
               ires = DOMGetNextObject(pLocalCurInfos->ObjectName,
                       pLocalCurInfos->ObjectOwner,pLocalCurInfos->ExtraData);
            }
            if (ires!=RES_ENDOFDATA)
               return ires;
         }
         break;
      default:
         return RES_ERR;
   }
   pCurRevInfos= pRevInfos;
   nbRevInfos  = nbobj;
   iRevObject  = 0;
   RevInfoType = iobjecttype;
   RevInfobSystem = bwithsystem;
   if (lpDBfilter)
      fstrncpy(DBFiltermem   , lpDBfilter,    MAXOBJECTNAME);
   else
      DBFiltermem[0]='\0';
   if (lpobjectfilter)
      fstrncpy(OtherFiltermem, lpobjectfilter,MAXOBJECTNAME);
   else
      OtherFiltermem[0]='\0';
   return DOMGetNextRelObject (resultparentstrings,lpresultobjectname, 
                               lpresultowner,lpresultextradata);
}

int  DOMGetNextRelObject (resultparentstrings, lpobjectname,
                          lpresultowner,  lpresultextrastring)

LPUCHAR *resultparentstrings;
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;     
LPUCHAR lpresultextrastring;
{
   int iresobjecttype=-1;
 
   switch (RevInfoType) {

      case OT_USER :
            return myerror(ERR_GW);
         break;
      case OTR_GRANTEE_ROLE  :
         iresobjecttype=OT_ROLE;
         break;
      case OTR_USERGROUP  :
         iresobjecttype=OT_GROUP;
         break;
      case OTR_REPLIC_TABLE_CDDS:
         iresobjecttype=OT_REPLIC_CDDS;
         break;
      case OTR_USERSCHEMA :
      case OTR_DBGRANT_ACCESY_DB :
      case OTR_DBGRANT_ACCESN_DB :
      case OTR_DBGRANT_CREPRY_DB :
      case OTR_DBGRANT_CREPRN_DB :
      case OTR_DBGRANT_CRETBY_DB :
      case OTR_DBGRANT_CRETBN_DB :
      case OTR_DBGRANT_DBADMY_DB :
      case OTR_DBGRANT_DBADMN_DB :
      case OTR_DBGRANT_LKMODY_DB :
      case OTR_DBGRANT_LKMODN_DB :
      case OTR_DBGRANT_QRYIOY_DB :
      case OTR_DBGRANT_QRYION_DB :
      case OTR_DBGRANT_QRYRWY_DB :
      case OTR_DBGRANT_QRYRWN_DB :
      case OTR_DBGRANT_UPDSCY_DB :
      case OTR_DBGRANT_UPDSCN_DB :
      case OTR_DBGRANT_SELSCY_DB : 
      case OTR_DBGRANT_SELSCN_DB : 
      case OTR_DBGRANT_CNCTLY_DB : 
      case OTR_DBGRANT_CNCTLN_DB : 
      case OTR_DBGRANT_IDLTLY_DB : 
      case OTR_DBGRANT_IDLTLN_DB : 
      case OTR_DBGRANT_SESPRY_DB : 
      case OTR_DBGRANT_SESPRN_DB : 
      case OTR_DBGRANT_TBLSTY_DB : 
      case OTR_DBGRANT_TBLSTN_DB : 
      case OTR_DBGRANT_QRYCPN_DB :
      case OTR_DBGRANT_QRYCPY_DB :
      case OTR_DBGRANT_QRYPGN_DB :
      case OTR_DBGRANT_QRYPGY_DB :
      case OTR_DBGRANT_QRYCON_DB :
      case OTR_DBGRANT_QRYCOY_DB :
      case OTR_DBGRANT_SEQCRN_DB :
      case OTR_DBGRANT_SEQCRY_DB :
         iresobjecttype=OT_GRANTEE;
         break;
      case OTR_TABLESYNONYM:
      case OTR_VIEWSYNONYM:
      case OTR_INDEXSYNONYM:
         iresobjecttype=OT_SYNONYM;
         break;
      case OTR_TABLEVIEW:
         iresobjecttype=OT_VIEW;
         break;
      case OTR_PROC_RULE:
         iresobjecttype=OT_RULE;
         break;
      case OTR_LOCATIONTABLE :
      case OTR_ALARM_SELSUCCESS_TABLE :
      case OTR_ALARM_SELFAILURE_TABLE :
      case OTR_ALARM_DELSUCCESS_TABLE :
      case OTR_ALARM_DELFAILURE_TABLE :
      case OTR_ALARM_INSSUCCESS_TABLE :
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :
      case OTR_GRANTEE_SEL_TABLE      :
      case OTR_GRANTEE_INS_TABLE      :
      case OTR_GRANTEE_UPD_TABLE      :
      case OTR_GRANTEE_DEL_TABLE      :
      case OTR_GRANTEE_REF_TABLE      :
      case OTR_GRANTEE_CPI_TABLE      :
      case OTR_GRANTEE_CPF_TABLE      :
      case OTR_GRANTEE_ALL_TABLE      :
      case OTR_REPLIC_CDDS_TABLE      :
         iresobjecttype=OT_TABLE;
         break;
      case OTR_GRANTEE_SEL_VIEW       :
      case OTR_GRANTEE_INS_VIEW       :
      case OTR_GRANTEE_UPD_VIEW       :
      case OTR_GRANTEE_DEL_VIEW       :
         iresobjecttype=OT_VIEW;
         break;
      case OTR_GRANTEE_EXEC_PROC      :
         iresobjecttype=OT_PROCEDURE;
         break;
      case OTR_GRANTEE_NEXT_SEQU      :
         iresobjecttype=OT_SEQUENCE;
         break;
      case OTR_GRANTEE_RAISE_DBEVENT  :
      case OTR_GRANTEE_REGTR_DBEVENT  :
         iresobjecttype=OT_DBEVENT;
         break;
	  case OTR_CDB:
         iresobjecttype=OT_DATABASE;
         break;
      default:
         return RES_ERR;

   }

   while (TRUE) {
      BOOL bContinue=FALSE;

      if (iRevObject >= nbRevInfos)
         return RES_ENDOFDATA;

      if (!lpobjectname)
         return myerror(ERR_DOMDATA);

      if (!pCurRevInfos) {
         myerror(ERR_LIST);
         return RES_ENDOFDATA;
      }
      if (HasOwner(iresobjecttype)) {
         LPUCHAR lp1;
         if (iresobjecttype==OT_DATABASE)
            lp1=DBFiltermem;
         else 
            lp1=OtherFiltermem;
         if (lp1[0] && x_strcmp(lp1,pCurRevInfos->ObjectOwner))
            bContinue=TRUE;
      }
      if (!RevInfobSystem &&
          IsSystemObject(iresobjecttype,
                         pCurRevInfos->ObjectName,
                         pCurRevInfos->ObjectOwner)
         )
         bContinue=TRUE;

      if (!bContinue)
         break;

      iRevObject++;
      pCurRevInfos = pCurRevInfos->pnext;

   }


   switch (RevInfoType) {

      case OTR_USERGROUP   :
      case OTR_GRANTEE_ROLE :
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         break;
      case OTR_USERSCHEMA :
      case OTR_DBGRANT_ACCESY_DB : /* same as OTR_USERSCHEMA */
      case OTR_DBGRANT_ACCESN_DB :
      case OTR_DBGRANT_CREPRY_DB :
      case OTR_DBGRANT_CREPRN_DB :
      case OTR_DBGRANT_CRETBY_DB :
      case OTR_DBGRANT_CRETBN_DB :
      case OTR_DBGRANT_DBADMY_DB :
      case OTR_DBGRANT_DBADMN_DB :
      case OTR_DBGRANT_LKMODY_DB :
      case OTR_DBGRANT_LKMODN_DB :
      case OTR_DBGRANT_QRYIOY_DB :
      case OTR_DBGRANT_QRYION_DB :
      case OTR_DBGRANT_QRYRWY_DB :
      case OTR_DBGRANT_QRYRWN_DB :
      case OTR_DBGRANT_UPDSCY_DB :
      case OTR_DBGRANT_UPDSCN_DB :
      case OTR_DBGRANT_SELSCY_DB : 
      case OTR_DBGRANT_SELSCN_DB : 
      case OTR_DBGRANT_CNCTLY_DB : 
      case OTR_DBGRANT_CNCTLN_DB : 
      case OTR_DBGRANT_IDLTLY_DB : 
      case OTR_DBGRANT_IDLTLN_DB : 
      case OTR_DBGRANT_SESPRY_DB : 
      case OTR_DBGRANT_SESPRN_DB : 
      case OTR_DBGRANT_TBLSTY_DB : 
      case OTR_DBGRANT_TBLSTN_DB : 
      case OTR_DBGRANT_QRYCPN_DB :
      case OTR_DBGRANT_QRYCPY_DB :
      case OTR_DBGRANT_QRYPGN_DB :
      case OTR_DBGRANT_QRYPGY_DB :
      case OTR_DBGRANT_QRYCON_DB :
      case OTR_DBGRANT_QRYCOY_DB :
      case OTR_DBGRANT_SEQCRN_DB :
      case OTR_DBGRANT_SEQCRY_DB :
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
         break;
      case OTR_TABLESYNONYM:
      case OTR_VIEWSYNONYM:
      case OTR_INDEXSYNONYM:
         {
           int ilev, nblev;
           nblev=2;
           if (RevInfoType==OTR_INDEXSYNONYM)
             nblev=3;
           fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
           if (lpresultowner)
              fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
           if (lpresultextrastring) 
              fstrncpy(lpresultextrastring,pCurRevInfos->ExtraData,
                       MAXOBJECTNAME);
           for (ilev=0;ilev<nblev;ilev++) {
              fstrncpy(resultparentstrings[ilev],
                       pCurRevInfos->Parents[ilev],MAXOBJECTNAME);
           }
           break;
         }
      case OTR_TABLEVIEW:
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
         fstrncpy(resultparentstrings[0],pCurRevInfos->Parents[0],MAXOBJECTNAME);
         if (lpresultextrastring) 
            fstrncpy(lpresultextrastring,pCurRevInfos->ExtraData,
                     MAXOBJECTNAME);
         break;
      case OTR_PROC_RULE:
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
         if (lpresultextrastring) 
            fstrncpy(lpresultextrastring,pCurRevInfos->ExtraData,
                     MAXOBJECTNAME);
         fstrncpy(resultparentstrings[0],pCurRevInfos->Parents[0],MAXOBJECTNAME);
         fstrncpy(resultparentstrings[1],pCurRevInfos->Parents[1],MAXOBJECTNAME);
         break;
      case OTR_REPLIC_TABLE_CDDS:
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            memcpy(lpresultowner,pCurRevInfos->ObjectOwner,STEPSMALLOBJ);
         fstrncpy(resultparentstrings[0],pCurRevInfos->Parents[0],MAXOBJECTNAME);
         break;
      case OTR_LOCATIONTABLE :
      case OTR_ALARM_SELSUCCESS_TABLE : /* same as for OTR_LOCATIONTABLE */
      case OTR_ALARM_SELFAILURE_TABLE :
      case OTR_ALARM_DELSUCCESS_TABLE :
      case OTR_ALARM_DELFAILURE_TABLE :
      case OTR_ALARM_INSSUCCESS_TABLE :
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :
      case OTR_REPLIC_CDDS_TABLE      :
      case OTR_GRANTEE_SEL_TABLE      :
      case OTR_GRANTEE_INS_TABLE      :
      case OTR_GRANTEE_UPD_TABLE      :
      case OTR_GRANTEE_DEL_TABLE      :
      case OTR_GRANTEE_REF_TABLE      :
      case OTR_GRANTEE_CPI_TABLE      :
      case OTR_GRANTEE_CPF_TABLE      :
      case OTR_GRANTEE_ALL_TABLE      :
      case OTR_GRANTEE_SEL_VIEW       :
      case OTR_GRANTEE_INS_VIEW       :
      case OTR_GRANTEE_UPD_VIEW       :
      case OTR_GRANTEE_DEL_VIEW       :
      case OTR_GRANTEE_EXEC_PROC      :
      case OTR_GRANTEE_NEXT_SEQU      :
      case OTR_GRANTEE_RAISE_DBEVENT  :
      case OTR_GRANTEE_REGTR_DBEVENT  :
         fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
         fstrncpy(resultparentstrings[0],pCurRevInfos->Parents[0],MAXOBJECTNAME);
         if (RevInfoType==OTR_GRANTEE_EXEC_PROC && lpresultextrastring)
          fstrncpy(lpresultextrastring,pCurRevInfos->ExtraData,MAXOBJECTNAME);
         switch (RevInfoType) {
            case OTR_ALARM_SELSUCCESS_TABLE : /* same as for OTR_LOCATIONTABLE */
            case OTR_ALARM_SELFAILURE_TABLE :
            case OTR_ALARM_DELSUCCESS_TABLE :
            case OTR_ALARM_DELFAILURE_TABLE :
            case OTR_ALARM_INSSUCCESS_TABLE :
            case OTR_ALARM_INSFAILURE_TABLE :
            case OTR_ALARM_UPDSUCCESS_TABLE :
            case OTR_ALARM_UPDFAILURE_TABLE :
              storeint(lpresultextrastring,getint(pCurRevInfos->ExtraData));
              break;
            default:
              break;
         }
         break;
      case OT_USER :
            return myerror(ERR_GW);
         break;
	  case OTR_CDB :
		  fstrncpy(lpobjectname,pCurRevInfos->ObjectName,MAXOBJECTNAME);
         if (lpresultowner)
            fstrncpy(lpresultowner,pCurRevInfos->ObjectOwner,MAXOBJECTNAME);
         if (lpresultextrastring)  {
            memcpy(lpresultextrastring,pCurRevInfos->ExtraData,2*STEPSMALLOBJ);
			x_strcpy(lpresultextrastring+2*STEPSMALLOBJ,pCurRevInfos->ExtraData+2*STEPSMALLOBJ);
         }
		 break;
      default:
         return RES_ERR;

   }

   iRevObject++;
   pCurRevInfos = pCurRevInfos->pnext;

   return RES_SUCCESS;

}

int DOMLocationUsageAccepted(hnodestruct,lpobjectname,usagetype,pReturn)
int       hnodestruct;
LPUCHAR   lpobjectname;
int       usagetype;
BOOL    * pReturn;
{
   int  ires;
   UCHAR buf[MAXOBJECTNAME];  
   UCHAR bufowner[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   if (usagetype<0 || usagetype>4)
      return RES_ERR;

   pdata1 = virtnode[hnodestruct].pdata;
   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   push4domg();/* allows to be called in a DomGetFirst/Next loop*/

   ires=DOMGetFirstObject ( hnodestruct, OT_LOCATIONWITHUSAGES,0,
      aparents,TRUE, (LPUCHAR)0,buf, bufowner,extradata);

   while (ires==RES_SUCCESS) {
      if (!x_strcmp(lpobjectname,buf)) {
         *pReturn=(extradata[MAXOBJECTNAME+usagetype]==ATTACHED_YES);
         break;
      }
      ires = DOMGetNextObject(buf, bufowner, extradata);
   }
   pop4domg();
   return ires;
}

int DOMGetDBOwner(int hnodestruct,LPUCHAR dbname, LPUCHAR resultdbowner)
{
   UCHAR buf      [MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   int ires;

   push4domg();      /* allows to be called in a DomGetFirst/Next loop*/
   ires=DOMGetFirstObject ( hnodestruct, OT_DATABASE,0,
      NULL,TRUE, (LPUCHAR)0,buf, resultdbowner,extradata);

   while (ires==RES_SUCCESS) {
      if (!x_stricmp(dbname,buf))
         break;
      ires = DOMGetNextObject(buf, resultdbowner, extradata);
   }
   pop4domg();
   return ires;
}

int DOMGetObjectLimitedInfo(hnodestruct,lpobjectname,lpobjectowner,iobjecttype,level,
              parentstrings, bwithsystem, presulttype, lpresultobjectname,
              lpresultowner, lpresultextradata)
int     hnodestruct;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bwithsystem;
int     *presulttype;
LPUCHAR lpresultobjectname;  
LPUCHAR lpresultowner;      
LPUCHAR lpresultextradata;
{
   LPUCHAR resultparentstrings[MAXPLEVEL];
   UCHAR buf[MAXPLEVEL][MAXOBJECTNAME];
   int i,resultlevel;
   for (i=0;i<MAXPLEVEL;i++) 
      resultparentstrings[i]=buf[i];
   return DOMGetObject(hnodestruct,lpobjectname,lpobjectowner,iobjecttype,level,parentstrings,
              bwithsystem,presulttype, &resultlevel, resultparentstrings,
              lpresultobjectname, lpresultowner, lpresultextradata);
}

int DOMGetObject(hnodestruct,lpobjectname,lpobjectowner, iobjecttype,level,parentstrings,
              bwithsystem,presulttype, presultlevel, resultparentstrings,
              lpresultobjectname, lpresultowner, lpresultextradata)
int     hnodestruct;
LPUCHAR lpobjectname;
LPUCHAR lpobjectowner;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bwithsystem;
int     *presulttype;
int     *presultlevel;
LPUCHAR *resultparentstrings;
LPUCHAR lpresultobjectname;  
LPUCHAR lpresultowner;      
LPUCHAR lpresultextradata;

{
   int  ires,ires1;
   UCHAR buf[MAXOBJECTNAME];  
   UCHAR bufparent[MAXOBJECTNAME];  
   UCHAR bufparentowner[MAXOBJECTNAME];  
   UCHAR bufowner[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   UCHAR Parent1Owner[MAXOBJECTNAME];
   struct ServConnectData * pdata1;

   extradata[0]=bufowner[0]='\0';

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;
   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   switch (iobjecttype) {

      case OT_GRANTEE:
         {
            int i;
            int itype[]={OT_USER,OT_GROUP,OT_ROLE} ;
            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               ires=DOMGetObject(hnodestruct,lpobjectname,lpobjectowner,itype[i],level,
                        parentstrings,bwithsystem,presulttype, presultlevel,
                        resultparentstrings,lpresultobjectname,lpresultowner,
                        lpresultextradata);
               if (ires!=RES_ENDOFDATA)
                  return ires;
            }
            return ires;
         }
         break;
      case OT_VIEWTABLE:
         /* same code as for OT_SYNONYM so no code /no break here */
      case OT_SYNONYMOBJECT:
         {
            int i,loctype;
            if (level !=1)
               return myerror(ERR_LEVELNUMBER);
            for (i=0,loctype=OT_TABLE;i<2;i++,loctype=OT_VIEW) {
               push4domg();/* allows to be called in a DomGetFirst/Next loop*/
               ires=DOMGetFirstObject ( hnodestruct, loctype,1, parentstrings,
                  bwithsystem, (LPUCHAR) 0,buf, bufowner,extradata);

               while (ires==RES_SUCCESS) {
                  BOOL bFound;
                  bFound=DOMTreeCmpTableNames(lpobjectname,lpobjectowner,
                                              buf,bufowner)?FALSE:TRUE;
                  if (bFound) {
                     fstrncpy(lpresultobjectname, buf, MAXOBJECTNAME);
                     if (lpresultowner)
                        fstrncpy(lpresultowner, bufowner, MAXOBJECTNAME);

                     // Emb in lieu of Fnn: may be star object ---> return star type
                     if (lpresultextradata)
                       memcpy(lpresultextradata, extradata, MAXOBJECTNAME);

                     * presulttype = loctype;
                     * presultlevel= 1;
                     fstrncpy(resultparentstrings[0],parentstrings[0],
                              MAXOBJECTNAME);
                     break;
                  }
                  ires = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires!=RES_ENDOFDATA) /* error */
                  return ires;
            }
            if (iobjecttype==OT_SYNONYMOBJECT) {
               push4domg();/* allows to be called in a DomGetFirst/Next loop*/
               aparents[0]=parentstrings[0];

               ires=DOMGetFirstObject (hnodestruct, OT_TABLE,1,parentstrings,
                   bwithsystem,(LPUCHAR)0,bufparent,bufparentowner,extradata);

               while (ires==RES_SUCCESS) {
                  push4domg();
                  aparents[1]=StringWithOwner(bufparent,bufparentowner,Parent1Owner);
                  ires1=DOMGetFirstObject(hnodestruct, OT_INDEX, 2, aparents,
                       bwithsystem, (LPUCHAR)0, buf, bufowner, extradata);
                  while (ires1==RES_SUCCESS) {
                     if (!DOMTreeCmpTableNames(lpobjectname,lpobjectowner,
                                            buf,bufowner)) {
                        fstrncpy(lpresultobjectname, buf, MAXOBJECTNAME);
                        if (lpresultowner)
                           fstrncpy(lpresultowner, bufowner, MAXOBJECTNAME);
                        * presulttype = OT_INDEX;
                        * presultlevel= 2;
                        fstrncpy(resultparentstrings[0],parentstrings[0], MAXOBJECTNAME);
                        fstrncpy(resultparentstrings[1],
                                 StringWithOwner(bufparent,bufparentowner,Parent1Owner),
                                 MAXOBJECTNAME);
                           break;
                     }
                     ires1 = DOMGetNextObject(buf, bufowner, extradata);
                  }
                  pop4domg();
                  if (ires1!=RES_ENDOFDATA){ /* found, or error */
                     ires = ires1;
                     break;
                  }
                  ires = DOMGetNextObject(bufparent,bufparentowner,extradata);
               }
               pop4domg();
            }
         }
         break;
      case OT_PROCEDURE:
      case OT_SEQUENCE:
      case OT_TABLE:
      case OT_VIEW:
      case OT_DBEVENT:
      case OT_SYNONYM:
      case OT_SCHEMAUSER:
      case OT_LOCATION:
      case OT_DATABASE:
      case OT_PROFILE:
      case OT_USER:
      case OT_ROLE:
      case OT_GROUP:
      case OT_REPLIC_CONNECTION :
         {
            int iloc,ilev;
            switch (iobjecttype) {
               case OT_PROCEDURE:
               case OT_SEQUENCE:
               case OT_TABLE:
               case OT_VIEW:
               case OT_DBEVENT:
               case OT_SYNONYM:
               case OT_SCHEMAUSER:
               case OT_REPLIC_CONNECTION :
                  ilev=1;
                  break;
               default:
                  ilev=0;
                  break;
            }
            if (level!=ilev)
               return myerror(ERR_LEVELNUMBER);

            push4domg();/* allows to be called in a DomGetFirst/Next loop*/
            ires=DOMGetFirstObject ( hnodestruct, iobjecttype,level,
               parentstrings,bwithsystem, (LPUCHAR)0,buf, bufowner,extradata);

            while (ires==RES_SUCCESS) {
               BOOL bFound=TRUE;
               if (CanObjectHaveSchema(iobjecttype))
                     bFound=DOMTreeCmpTableNames(lpobjectname,lpobjectowner,
                                            buf,bufowner)?FALSE:TRUE;
               else
                  bFound=x_strcmp(lpobjectname,buf)?FALSE:TRUE;
               if (bFound) {
                  fstrncpy(lpresultobjectname, buf, MAXOBJECTNAME);
                  if (lpresultowner) {  /* some non-string data may be stored here. Do not use fstrncpy */
                     memcpy(lpresultowner, bufowner, MAXOBJECTNAME);
                     lpresultowner[MAXOBJECTNAME-1]='\0';
                  }
                  if (lpresultextradata)
                     memcpy(lpresultextradata, extradata, MAXOBJECTNAME);
                  * presulttype = iobjecttype;
                  * presultlevel= ilev;
                  for (iloc=0;iloc<ilev;iloc++)
                     fstrncpy(resultparentstrings[iloc],parentstrings[iloc],
                              MAXOBJECTNAME);
                  break;
               }
               ires = DOMGetNextObject(buf, bufowner, extradata);
            }
            pop4domg();
         }
         break;
      case OT_INDEX:
      case OT_INTEGRITY:
      case OT_RULE:
         {
            BOOL bFound;
            int iintegrity2find;
            if (level!=2)
               return myerror(ERR_LEVELNUMBER);
            if (iobjecttype==OT_INTEGRITY)
               iintegrity2find = atoi(lpobjectname);

            push4domg();/* allows to be called in a DomGetFirst/Next loop*/
            aparents[0]=parentstrings[0];

            ires=DOMGetFirstObject (hnodestruct, OT_TABLE,1,parentstrings,
               bwithsystem, (LPUCHAR) 0,bufparent, bufparentowner, extradata);

            while (ires==RES_SUCCESS) {
               push4domg();
               aparents[1]=StringWithOwner(bufparent,bufparentowner,Parent1Owner);
               ires1=DOMGetFirstObject(hnodestruct, iobjecttype, 2, aparents,
                    bwithsystem, (LPUCHAR)0, buf, bufowner, extradata);
               while (ires1==RES_SUCCESS) {
                  if (iobjecttype==OT_INTEGRITY) {
                     int iintegrity = getint(extradata);
                     bFound=(iintegrity==iintegrity2find);
                  }
                  else
                     bFound= DOMTreeCmpTableNames(lpobjectname,lpobjectowner,
                                            buf,bufowner)?FALSE:TRUE;
                  if (bFound) {
                     fstrncpy(lpresultobjectname, buf, MAXOBJECTNAME);
                     if (lpresultowner)
                        fstrncpy(lpresultowner, bufowner, MAXOBJECTNAME);
                     if (lpresultextradata)
                        memcpy(lpresultextradata,extradata, MAXOBJECTNAME);
                     * presulttype = iobjecttype;
                     * presultlevel= 2;
                     fstrncpy(resultparentstrings[0],parentstrings[0], MAXOBJECTNAME);
                     fstrncpy(resultparentstrings[1],
                              StringWithOwner(bufparent,bufparentowner,Parent1Owner),
                              MAXOBJECTNAME);
                     break;
                  }
                  ires1 = DOMGetNextObject(buf, bufowner, extradata);
               }
               pop4domg();
               if (ires1!=RES_ENDOFDATA){ /* found, or error */
                  ires = ires1;
                  break;
               }
               ires = DOMGetNextObject(bufparent, bufparentowner, extradata);
            }
            pop4domg();
         }
         break;
      default:
         return RES_ERR;
   }
   if (ires!=RES_SUCCESS && !bwithsystem)
      return DOMGetObject(hnodestruct,lpobjectname,lpobjectowner, iobjecttype,level,
          parentstrings,TRUE,presulttype, presultlevel, resultparentstrings,
              lpresultobjectname, lpresultowner, lpresultextradata);
   return ires;
}

int isDBGranted(hnodestruct,lpDBName,lpgranteename,pReturn)
int       hnodestruct;
LPUCHAR   lpDBName;
LPUCHAR   lpgranteename;
BOOL    * pReturn;
{
   int     ires;
   UCHAR   buf[MAXOBJECTNAME];  
   UCHAR   bufowner[MAXOBJECTNAME];
   UCHAR   extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;
   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   *pReturn = FALSE;

   push4domg();/* allows to be called in a DomGetFirst/Next loop*/

   aparents[0] = lpDBName;
   ires=DOMGetFirstObject(hnodestruct,OT_DBGRANT_ACCESY_USER,1,aparents,
                          TRUE, (LPUCHAR)0, buf, bufowner, extradata);
   while (ires==RES_SUCCESS) {
      if (!x_strcmp(lpgranteename,buf)) {
         *pReturn = TRUE;
         break;
      }
      ires = DOMGetNextObject(buf, bufowner, extradata);
   }

   pop4domg();
   return ires;
}


int HasDBNoaccess(hnodestruct,lpDBName,lpgranteename,pReturn)
int       hnodestruct;
LPUCHAR   lpDBName;
LPUCHAR   lpgranteename;
BOOL    * pReturn;
{
   int     ires;
   UCHAR   buf[MAXOBJECTNAME];  
   UCHAR   bufowner[MAXOBJECTNAME];
   UCHAR   extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   struct ServConnectData * pdata1;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;
   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   *pReturn = FALSE;

   push4domg();/* allows to be called in a DomGetFirst/Next loop*/

   aparents[0] = lpDBName;
   ires=DOMGetFirstObject(hnodestruct,OT_DBGRANT_ACCESN_USER,1,aparents,
                          TRUE, (LPUCHAR)0, buf, bufowner, extradata);
   while (ires==RES_SUCCESS) {
      if (!x_strcmp(lpgranteename,buf)) {
         *pReturn = TRUE;
         break;
      }
      ires = DOMGetNextObject(buf, bufowner, extradata);
   }

   pop4domg();
   return ires;
}
BOOL ExistRemoveSessionProc(int hnode)
{
  int irestype,ires;
  CHAR buf     [MAXOBJECTNAME];
  CHAR bufowner[MAXOBJECTNAME];
  CHAR extradata[EXTRADATASIZE];
  LPUCHAR parents[MAXPLEVEL];
  parents[0]="imadb";

  ires= DOMGetObjectLimitedInfo(hnode,    "ima_remove_session",
                                          "$ingres",
                                          OT_PROCEDURE,
                                          1,
                                          parents,
                                          TRUE,
                                          &irestype,
                                          buf,
                                          bufowner,
                                          extradata);
   if (ires==RES_SUCCESS)
      return TRUE;
   return FALSE;
}

BOOL ExistShutDownSvr( int hnode)
{
  int irestype,ires;
  CHAR buf     [MAXOBJECTNAME];
  CHAR bufowner[MAXOBJECTNAME];
  CHAR extradata[EXTRADATASIZE];
  LPUCHAR parents[MAXPLEVEL];
  parents[0]="imadb";

  ires= DOMGetObjectLimitedInfo(hnode,    "ima_shut_server",
                                          "$ingres",
                                          OT_PROCEDURE,
                                          1,
                                          parents,
                                          TRUE,
                                          &irestype,
                                          buf,
                                          bufowner,
                                          extradata);
   if (ires==RES_SUCCESS)
      return TRUE;
   return FALSE;

}

BOOL ExistProcedure4OpenCloseSvr( int hnode)
{
  int irestype,ires1,ires2;
  CHAR buf     [MAXOBJECTNAME];
  CHAR bufowner[MAXOBJECTNAME];
  CHAR extradata[EXTRADATASIZE];
  LPUCHAR parents[MAXPLEVEL];
  parents[0]="imadb";

  ires1= DOMGetObjectLimitedInfo(hnode,    "ima_open_server",
                                          "$ingres",
                                          OT_PROCEDURE,
                                          1,
                                          parents,
                                          TRUE,
                                          &irestype,
                                          buf,
                                          bufowner,
                                          extradata);

  ires2= DOMGetObjectLimitedInfo(hnode,    "ima_close_server",
                                          "$ingres",
                                          OT_PROCEDURE,
                                          1,
                                          parents,
                                          TRUE,
                                          &irestype,
                                          buf,
                                          bufowner,
                                          extradata);
   if (ires1==RES_SUCCESS && ires2==RES_SUCCESS)
      return TRUE;
   return FALSE;
}
