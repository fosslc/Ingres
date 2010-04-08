/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : domdgetd.c
**    get information from cache
**
** History
**
**	21-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\domdgetd.c source
**  24-Mar-2004 (shaha03)
**		(sir 112040) Added the Multi-thread ability to the LibMon directory. 
**		(sir 112040) Global variables stack4domg,dominfo are changed to stack4domg[MAX_THREADS},
**		dominfo[MAX_THREADS] to support multiple threads
**		(sir 112040) prototypr of push4domg() is changed to push4domg(int thdl)
**		(sir 112040) prototypr of pop4domg() is changed to pop4domg(int thdl)
**		(sir 112040) added LIBMON_DOMGetFirstObject() function added with new parameter "int thdl" to support multi threaded,
**		for the same purpose of DOMGetFirstObject()
********************************************************************/

#include <tchar.h>
#include "libmon.h"
#include "domdloca.h"
#include "error.h"
#include "lbmnmtx.h"


struct ResultReverseInfos {
   UCHAR ObjectName [MAXOBJECTNAME];
   UCHAR ObjectOwner[MAXOBJECTNAME];
   UCHAR ExtraData[EXTRADATASIZE];
   UCHAR Parents[MAXPLEVEL][MAXOBJECTNAME];

   struct ResultReverseInfos * pnext;
};

static char * lperrusr = "<Unavailable List>";

extern mon4struct	monInfo[MAX_THREADS];
#define MAXLOOP 8

typedef struct{
	int stacklevel;
	int iObject;
	int iobjecttypemem;
	BOOL bsystemmem;
	void *pmemstruct;
	int nbelemmem;
	char lpownermem   [MAXOBJECTNAME];
	char lpllfiltermem[MAXOBJECTNAME];
	char par0         [MAXOBJECTNAME];
	int  hnodemem;
	int  lltypemem;
}dom4struct;
/**/

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

struct	type_stack4domg stack4domg[MAXLOOP][MAX_THREADS];
dom4struct dominfo[MAX_THREADS];


void push4domg (int thdl)
{
   if (dominfo[thdl].stacklevel>=MAXLOOP) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   stack4domg[dominfo[thdl].stacklevel][thdl].iObject        = dominfo[thdl].iObject        ;
   stack4domg[dominfo[thdl].stacklevel][thdl].iobjecttypemem = dominfo[thdl].iobjecttypemem ;
   stack4domg[dominfo[thdl].stacklevel][thdl].bsystemmem     = dominfo[thdl].bsystemmem     ;
   stack4domg[dominfo[thdl].stacklevel][thdl].pmemstruct     = dominfo[thdl].pmemstruct     ;
   stack4domg[dominfo[thdl].stacklevel][thdl].nbelemmem      = dominfo[thdl].nbelemmem      ;
   fstrncpy(stack4domg[dominfo[thdl].stacklevel][thdl].lpownermem,   dominfo[thdl].lpownermem ,  MAXOBJECTNAME);
   fstrncpy(stack4domg[dominfo[thdl].stacklevel][thdl].lpllfiltermem,dominfo[thdl].lpllfiltermem,MAXOBJECTNAME);
   fstrncpy(stack4domg[dominfo[thdl].stacklevel][thdl].par0,         dominfo[thdl].par0,         MAXOBJECTNAME);
   stack4domg[dominfo[thdl].stacklevel][thdl].hnodemem       = dominfo[thdl].hnodemem       ;
   stack4domg[dominfo[thdl].stacklevel][thdl].lltypemem      = dominfo[thdl].lltypemem      ;
   dominfo[thdl].stacklevel++;
}

void pop4domg (int thdl)
{
   dominfo[thdl].stacklevel--;
   if (dominfo[thdl].stacklevel<0) {
      myerror(ERR_LEVELNUMBER);
      return;
   }
   dominfo[thdl].iObject        = stack4domg[dominfo[thdl].stacklevel][thdl].iObject        ;
   dominfo[thdl].iobjecttypemem = stack4domg[dominfo[thdl].stacklevel][thdl].iobjecttypemem ;
   dominfo[thdl].bsystemmem     = stack4domg[dominfo[thdl].stacklevel][thdl].bsystemmem     ;
   dominfo[thdl].pmemstruct     = stack4domg[dominfo[thdl].stacklevel][thdl].pmemstruct     ;
   dominfo[thdl].nbelemmem      = stack4domg[dominfo[thdl].stacklevel][thdl].nbelemmem      ;
   fstrncpy(dominfo[thdl].lpownermem ,  stack4domg[dominfo[thdl].stacklevel][thdl].lpownermem,   MAXOBJECTNAME);
   fstrncpy(dominfo[thdl].lpllfiltermem,stack4domg[dominfo[thdl].stacklevel][thdl].lpllfiltermem,MAXOBJECTNAME);
   fstrncpy(dominfo[thdl].par0,         stack4domg[dominfo[thdl].stacklevel][thdl].par0,         MAXOBJECTNAME);
   dominfo[thdl].hnodemem       = stack4domg[dominfo[thdl].stacklevel][thdl].hnodemem       ;
   dominfo[thdl].lltypemem      = stack4domg[dominfo[thdl].stacklevel][thdl].lltypemem      ;
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
	/* 0 is the default thread handle for single thread application */
	return LIBMON_DOMGetFirstObject  ( hnodestruct, iobjecttype, level, parentstrings,
                          bWithSystem, lpowner, lpobjectname, lpresultowner,
                          lpresultextrastring,0);
}


int  LIBMON_DOMGetFirstObject  ( hnodestruct, iobjecttype, level, parentstrings,
                          bWithSystem, lpowner, lpobjectname, lpresultowner,
                          lpresultextrastring, thdl)
int hnodestruct;
int iobjecttype;
int level;
LPUCHAR *parentstrings;
BOOL bWithSystem;
LPUCHAR lpowner;
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;
LPUCHAR lpresultextrastring;
int		thdl;
{
   int   i, ires;

   struct ServConnectData * pdata1;

   dominfo[thdl].iobjecttypemem    = iobjecttype;
   dominfo[thdl].bsystemmem        = bWithSystem;
   dominfo[thdl].hnodemem          = hnodestruct;

   dominfo[thdl].par0[0]='\0';
   if (level && parentstrings)  {
      if (parentstrings[0])
         fstrncpy(dominfo[thdl].par0,parentstrings[0], MAXOBJECTNAME);
   }

   if (lpowner) {
      if (x_stricmp(lpowner,lperrusr))
         fstrncpy(dominfo[thdl].lpownermem,lpowner, MAXOBJECTNAME);
      else 
         dominfo[thdl].lpownermem[0]='\0';
   }
   else
      dominfo[thdl].lpownermem[0]='\0';
   dominfo[thdl].lpllfiltermem[0]='\0';
   dominfo[thdl].lltypemem=0;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;

   /* load parent(s) if not done for "locate" */
   
   switch (iobjecttype) {
      case OT_TABLE:
      case OT_VIEW:
      case OT_VIEWALL:
      case OT_PROCEDURE:
      case OT_SCHEMAUSER:
      case OT_SYNONYM:
      case OT_DBEVENT:

      case OT_INDEX:
      case OT_INTEGRITY:
      case OT_RULE:
         if (!(pdata1->lpDBData)) {
            ires=LIBMON_UpdateDOMData(hnodestruct,OT_DATABASE,0,parentstrings,
                               bWithSystem,FALSE, thdl);
            if (ires != RES_SUCCESS)
               return ires;
         }
         break;
      case OT_MON_LOCKLIST_LOCK        :
         if (!(pdata1->lpLockListData)){
            ires=LIBMON_UpdateDOMData(hnodestruct,OT_MON_LOCKLIST,0,parentstrings,
                               bWithSystem,FALSE, thdl);
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
            ires=LIBMON_UpdateDOMData(hnodestruct,OT_MON_SERVER,0,parentstrings,
                               bWithSystem,FALSE, thdl);
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
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
	  case OT_MON_REPLIC_SERVER  :
#endif
      case OT_MON_ICE_CONNECTED_USER:
      case OT_MON_ICE_TRANSACTION   :
      case OT_MON_ICE_CURSOR        :
      case OT_MON_ICE_ACTIVE_USER   :
      case OT_MON_ICE_FILEINFO      :
      case OT_MON_ICE_DB_CONNECTION :

         if (!(pdata1->lpDBData)) {
            ires=LIBMON_UpdateDOMData(hnodestruct,OT_DATABASE,0,parentstrings,
                               bWithSystem,FALSE, thdl);
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
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->serverserrcode != RES_SUCCESS)
               return pdata1->serverserrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpServerData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbservers;
            break;
         }
      case OT_MON_LOCKLIST       :
         {
            if (!pdata1->lpLockListData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->LockListserrcode != RES_SUCCESS)
               return pdata1->LockListserrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpLockListData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nblocklists;
            break;
         }
      case OTLL_MON_RESOURCE       :
         {
            if (!pdata1->lpResourceData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->resourceserrcode != RES_SUCCESS)
               return pdata1->resourceserrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpResourceData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbresources;
            break;
         }
      case OT_MON_LOGPROCESS     :
         {
            if (!pdata1->lpLogProcessData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->logprocesseserrcode != RES_SUCCESS)
               return pdata1->logprocesseserrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpLogProcessData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbLogProcesses;
            break;
         }
      case OT_MON_LOGDATABASE    :
         {
            if (!pdata1->lpLogDBData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->logDBerrcode != RES_SUCCESS)
               return pdata1->logDBerrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpLogDBData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbLogDB;
            break;
         }
      case OT_MON_TRANSACTION:
         {
            if (!pdata1->lpLogTransactData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->LogTransacterrcode != RES_SUCCESS)
               return pdata1->LogTransacterrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpLogTransactData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbLogTransact;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpServerSessionData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->ServerSessionserrcode != RES_SUCCESS)
                     return pServerdata->ServerSessionserrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpServerSessionData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbServerSessions;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceConnUsersData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->iceConnuserserrcode != RES_SUCCESS)
                     return pServerdata->iceConnuserserrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceConnUsersData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbiceconnusers;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceActiveUsersData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->iceactiveuserserrcode != RES_SUCCESS)
                     return pServerdata->iceactiveuserserrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceActiveUsersData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbiceactiveusers;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceTransactionsData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icetransactionserrcode != RES_SUCCESS)
                     return pServerdata->icetransactionserrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceTransactionsData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbicetransactions;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceCursorsData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icecursorsserrcode != RES_SUCCESS)
                     return pServerdata->icecursorsserrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceCursorsData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbicecursors;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceCacheInfoData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icecacheinfoerrcode != RES_SUCCESS)
                     return pServerdata->icecacheinfoerrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceCacheInfoData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbicecacheinfo;
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
               if (!LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) {
                  if (!pServerdata->lpIceDbConnectData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pServerdata->icedbconnectinfoerrcode != RES_SUCCESS)
                     return pServerdata->icedbconnectinfoerrcode;
                  dominfo[thdl].pmemstruct=(void *) pServerdata->lpIceDbConnectData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pServerdata->nbicedbconnectinfo;
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
               if (!LIBMON_CompareMonInfo(OT_MON_LOCKLIST, parentstrings,&(pLockListdata->LockListDta), thdl)) { 
                  if (!pLockListdata->lpLockData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
                                        parentstrings,bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pLockListdata->lockserrcode != RES_SUCCESS)
                     return pLockListdata->lockserrcode;
                  dominfo[thdl].pmemstruct=(void *) pLockListdata->lpLockData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pLockListdata->nbLocks;
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
//                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,
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
      case OT_DATABASE  :
         {
            if (!pdata1->lpDBData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                  bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->DBerrcode != RES_SUCCESS)
               return pdata1->DBerrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpDBData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbdb;
            break;
         }
      case OT_USER      :
         {
            if (!pdata1->lpUsersData) {
               ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                             bWithSystem,FALSE, thdl);
               if (ires != RES_SUCCESS)
                  return ires;
            }
            if (pdata1->userserrcode != RES_SUCCESS)
               return pdata1->userserrcode;
            dominfo[thdl].pmemstruct = (void *) pdata1->lpUsersData;
            if (!dominfo[thdl].pmemstruct) 
               return myerror(ERR_UPDATEDOMDATA1);

            dominfo[thdl].nbelemmem=pdata1->nbusers;
            break;
         }
      case OT_TABLE     :
         {
            struct DBData * pDBdata;

            pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(parentstrings[0],pDBdata->DBName)) {
                  if (!pDBdata->lpTableData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->tableserrcode != RES_SUCCESS)
                     return pDBdata->tableserrcode;
                  dominfo[thdl].pmemstruct = (void *) pDBdata->lpTableData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pDBdata->nbtables;
                  goto getobj;
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
                           ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE, thdl);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->columnserrcode != RES_SUCCESS)
                           return ptabledata->columnserrcode;
                        dominfo[thdl].pmemstruct = (void *) ptabledata->lpColumnData;
                        if (!dominfo[thdl].pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        dominfo[thdl].nbelemmem=ptabledata->nbcolumns;
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
                           ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE, thdl);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (pViewdata->columnserrcode != RES_SUCCESS)
                           return pViewdata->columnserrcode;
                        dominfo[thdl].pmemstruct = (void *) pViewdata->lpColumnData;
                        if (!dominfo[thdl].pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        dominfo[thdl].nbelemmem=pViewdata->nbcolumns;
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
                           ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,
                                              level,parentstrings,bWithSystem,
                                              FALSE, thdl);
                           if (ires != RES_SUCCESS)
                              return ires;
                        }
                        if (ptabledata->indexeserrcode != RES_SUCCESS)
                           return ptabledata->indexeserrcode;
                        ptabledata->indexesused = TRUE;
                        dominfo[thdl].pmemstruct = (void *) ptabledata->lpIndexData;
                        if (!dominfo[thdl].pmemstruct) 
                           return myerror(ERR_UPDATEDOMDATA1);

                        dominfo[thdl].nbelemmem=ptabledata->nbIndexes;
                        goto getobj;
                     }
                  }
                  return myerror(ERR_PARENTNOEXIST);
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
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->dbeventserrcode != RES_SUCCESS)
                     return pDBdata->dbeventserrcode;
                  dominfo[thdl].pmemstruct = (void *) pDBdata->lpDBEventData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pDBdata->nbDBEvents;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
      case OT_MON_REPLIC_SERVER  :
         {
            struct DBData * pDBdata;
			char buf1[100];
			pDBdata = pdata1->lpDBData;
            if (!pDBdata) 
               return myerror(ERR_DOMGETDATA);
            for (i=0;i<pdata1->nbdb;i++,pDBdata=GetNextDB(pDBdata)) {
               if (!x_strcmp(LIBMON_GetMonInfoName(hnodestruct, OT_DATABASE, parentstrings, buf1,sizeof(buf1)-1, thdl),
			               pDBdata->DBName)) {
                  if (!pDBdata->lpMonReplServerData) {
                     ires=LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                                   bWithSystem,FALSE, thdl);
                     if (ires != RES_SUCCESS)
                        return ires;
                  }
                  if (pDBdata->monreplservererrcode != RES_SUCCESS)
                     return pDBdata->monreplservererrcode;
                  dominfo[thdl].pmemstruct = (void *) pDBdata->lpMonReplServerData;
                  if (!dominfo[thdl].pmemstruct) 
                     return myerror(ERR_UPDATEDOMDATA1);

                  dominfo[thdl].nbelemmem=pDBdata->nbmonreplservers;
                  goto getobj;
               }
            }
            return myerror(ERR_PARENTNOEXIST);
         }
         break;
#endif
      default:
         return RES_ERR;
         break;
   }
getobj:
	dominfo[thdl].iObject=0;
   return LIBMON_DOMGetNextObject(lpobjectname, lpresultowner,lpresultextrastring, thdl);
}

int  DOMGetNextObject( lpobjectname, lpresultowner,lpresultextrastring)
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;
LPUCHAR lpresultextrastring;
{
	/* 0 is the default thread handle for single thread application */
	return LIBMON_DOMGetNextObject( lpobjectname, lpresultowner,lpresultextrastring,0);

}


int  LIBMON_DOMGetNextObject( lpobjectname, lpresultowner,lpresultextrastring,thdl)
LPUCHAR lpobjectname;
LPUCHAR lpresultowner;
LPUCHAR lpresultextrastring;
int		thdl;
{
   UCHAR ownerforfilter[MAXOBJECTNAME];

   int iobjecttype         = dominfo[thdl].iobjecttypemem; 
   LPUCHAR lpowner = (LPUCHAR) dominfo[thdl].lpownermem;

   if (!lpresultowner)
      lpresultowner=ownerforfilter;

   if (lpowner && !lpowner[0])
      lpowner=(LPUCHAR)0;

   if (!lpobjectname)
      return myerror(ERR_DOMDATA);

   while (TRUE) {

      if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
         return RES_ENDOFDATA;

      if (!dominfo[thdl].pmemstruct) {
         myerror(ERR_LIST);
         return RES_ENDOFDATA;
      }

      switch (iobjecttype) {
// monitoring
         case OT_MON_SERVER         :
            {
               LPSERVERDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPSERVERDATAMIN) lpobjectname ) = pdata->ServerDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOCKLIST       :
            {
               LPLOCKLISTDATA pdata = dominfo[thdl].pmemstruct;
               while (pdata->LockListDta.bIs4AllLockLists)  {
                  dominfo[thdl].iObject++;
                  if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
                     return RES_ENDOFDATA;
                  pdata=pdata->pnext;
               }
               * ( (LPLOCKLISTDATAMIN) lpobjectname ) = pdata->LockListDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OTLL_MON_RESOURCE       :
            {
               LPRESOURCEDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPRESOURCEDATAMIN) lpobjectname ) = pdata->ResourceDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOGPROCESS     :
            {
               LPLOGPROCESSDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPLOGPROCESSDATAMIN) lpobjectname ) = pdata->LogProcessDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOGDATABASE    :
            {
               LPLOGDBDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPLOGDBDATAMIN) lpobjectname ) = pdata->LogDBDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_TRANSACTION:
            {
               LPLOGTRANSACTDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPLOGTRANSACTDATAMIN) lpobjectname ) = pdata->LogTransactDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_SESSION        :
            {
               LPSESSIONDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPSESSIONDATAMIN) lpobjectname ) = pdata->SessionDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_CONNECTED_USER:
            {
               LPICECONNUSERDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_CONNECTED_USERDATAMIN) lpobjectname ) = pdata->IceConnUserDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_ACTIVE_USER   :
            {
               LPICEACTIVEUSERDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_ACTIVE_USERDATAMIN) lpobjectname ) = pdata->IceActiveUserDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_TRANSACTION   :
            {
               LPICETRANSACTDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_TRANSACTIONDATAMIN) lpobjectname ) = pdata->IceTransactDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_CURSOR        :
            {
               LPICECURSORDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_CURSORDATAMIN) lpobjectname ) = pdata->IceCursorDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_FILEINFO      :
            {
               LPICECACHEDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_CACHEINFODATAMIN) lpobjectname ) = pdata->IceCacheDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_ICE_DB_CONNECTION :
            {
               LPICEDBCONNECTDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPICE_DB_CONNECTIONDATAMIN) lpobjectname ) = pdata->IceDbConnectDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
         case OT_MON_LOCK           :
         case OT_MON_LOCKLIST_LOCK  :
         case OT_MON_RES_LOCK   :
            {
               LPLOCKDATA pdata = dominfo[thdl].pmemstruct;
               * ( (LPLOCKDATAMIN) lpobjectname ) = pdata->LockDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
            }
// end of monitoring
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
		 case OT_MON_REPLIC_SERVER  :
			{
			   LPMONREPLSERVERDATA pdata=dominfo[thdl].pmemstruct;
               * ( (LPREPLICSERVERDATAMIN) lpobjectname ) = pdata->MonReplServerDta;
               dominfo[thdl].pmemstruct = pdata->pnext;
               break;
			}
#endif
         case OT_DATABASE  :
            {
               struct DBData * pDBData = dominfo[thdl].pmemstruct;

               while (lpowner && x_strcmp(pDBData->DBOwner,lpowner)) {
                  dominfo[thdl].iObject++;
                  if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
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

               dominfo[thdl].pmemstruct = pDBData->pnext;
               break;
            }
         case OT_USER      :
            {
               struct UserData * puserdata = dominfo[thdl].pmemstruct;

               fstrncpy(lpobjectname,puserdata->UserName,
                        sizeof(puserdata->UserName));
               
               if (lpresultextrastring) {
                  if (puserdata->bAuditAll)
                     lpresultextrastring[0]=0x01;
                  else
                     lpresultextrastring[0]=0x00;
                  lpresultextrastring[1]=0x00;
               }


               dominfo[thdl].pmemstruct = puserdata->pnext;
               break;
            }
         case OT_TABLE     :
            {
               struct TableData * pTableData = dominfo[thdl].pmemstruct;

               while (lpowner && x_strcmp(pTableData->TableOwner,lpowner)) {
                  dominfo[thdl].iObject++;
                  if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
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
               dominfo[thdl].pmemstruct = pTableData->pnext;
               break;
            }
         case OT_VIEW      :
         case OT_VIEWALL   :
            {
               struct ViewData * pViewData = dominfo[thdl].pmemstruct;

               while (lpowner && x_strcmp(pViewData->ViewOwner,lpowner)) {
                  dominfo[thdl].iObject++;
                  if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
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
               dominfo[thdl].pmemstruct = pViewData->pnext;
               break;
            }
         case OT_INTEGRITY :
            {
               struct IntegrityData * pIntegrityData = dominfo[thdl].pmemstruct;

               fstrncpy(lpobjectname,pIntegrityData->IntegrityName,
                        sizeof(pIntegrityData->IntegrityName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pIntegrityData->IntegrityOwner,
                           sizeof(pIntegrityData->IntegrityOwner));
               }
               if (lpresultextrastring)
                  storeint(lpresultextrastring,
                           (int)pIntegrityData->IntegrityNumber);
               dominfo[thdl].pmemstruct = pIntegrityData->pnext;
               break;
            }
            break;
         case OT_VIEWCOLUMN :
         case OT_COLUMN :
            {
               struct ColumnData * pColumnData = dominfo[thdl].pmemstruct;

               fstrncpy(lpobjectname,pColumnData->ColumnName,
                        sizeof(pColumnData->ColumnName));
               
               if (lpresultextrastring) {
                     storeint(lpresultextrastring,pColumnData->ColumnType);
                     storeint(lpresultextrastring+STEPSMALLOBJ,
                              pColumnData->ColumnLen);
               }
               dominfo[thdl].pmemstruct = pColumnData->pnext;
               break;
            }
            break;
         case OT_INDEX :
            {
               struct IndexData * pIndexData = dominfo[thdl].pmemstruct;

               fstrncpy(lpobjectname,pIndexData->IndexName,MAXOBJECTNAME);
               if (lpresultowner) 
                  fstrncpy(lpresultowner,pIndexData->IndexOwner,MAXOBJECTNAME);
               if (lpresultextrastring) {
                  fstrncpy(lpresultextrastring,pIndexData->IndexStorage,
                                               23);
                  storeint(lpresultextrastring+24,pIndexData->IndexId);
               }
               dominfo[thdl].pmemstruct = pIndexData->pnext;
               break;
            }
            break;
         case OT_DBEVENT :
            {
               struct DBEventData * pDBEventData = dominfo[thdl].pmemstruct;

               while (lpowner && x_strcmp(pDBEventData->DBEventOwner,lpowner)) {
                  dominfo[thdl].iObject++;
                  if (dominfo[thdl].iObject >= dominfo[thdl].nbelemmem)
                     return RES_ENDOFDATA;
                  pDBEventData=pDBEventData->pnext;
               }
               fstrncpy(lpobjectname,pDBEventData->DBEventName,
                        sizeof(pDBEventData->DBEventName));
               
               if (lpresultowner) {
                  fstrncpy(lpresultowner,pDBEventData->DBEventOwner,
                           sizeof(pDBEventData->DBEventOwner));
               }

               dominfo[thdl].pmemstruct = pDBEventData->pnext;
               break;
            }
         case OT_VIEWTABLE :
            {
               struct ViewTableData * pViewTable = dominfo[thdl].pmemstruct;

               fstrncpy(lpobjectname,pViewTable->TableName,
                        sizeof(pViewTable->TableName));
               fstrncpy(lpresultowner,pViewTable->TableOwner,
                        sizeof(pViewTable->TableOwner));
               
               dominfo[thdl].bsystemmem = TRUE;
               dominfo[thdl].pmemstruct = pViewTable->pnext;
               break;
            }
            break;
         default:
            return RES_ERR;
      }
      dominfo[thdl].iObject++;
      if (dominfo[thdl].bsystemmem ||
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
   int  ires;
   UCHAR buf[MAXOBJECTNAME];  
   UCHAR bufowner[MAXOBJECTNAME];
   UCHAR extradata[EXTRADATASIZE];
   struct ServConnectData * pdata1;
   int thdl=0; /*Default handle for non-threaded*/

   extradata[0]=bufowner[0]='\0';

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;
   if (!pdata1) 
      return myerror(ERR_UPDATEDOMDATA);

   switch (iobjecttype) {

      case OT_TABLE:
      case OT_VIEW:
      case OT_DBEVENT:
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

            push4domg(thdl);/* allows to be called in a DomGetFirst/Next loop*/
            ires=LIBMON_DOMGetFirstObject ( hnodestruct, iobjecttype,level,
               parentstrings,bwithsystem, (LPUCHAR)0,buf, bufowner,extradata, thdl);

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
               ires = LIBMON_DOMGetNextObject(buf, bufowner, extradata, thdl);
            }
            pop4domg(thdl);
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

