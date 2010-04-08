/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : domdupdt.c
**    update cache from low level
**
** History
**
**	22-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\domdupdt.c source
**  24-Mar-2004 (shaha03)
**		(sir 112040) changed "static int UpdateDOMDataDetail1()" prototype and added 
**			a new parameter "int thdl" to support the multi-threading
**		(sir 112040) Added "LIBMON_UpdateDOMData()"  with a new paramater "int thdl" to support thread, 
**			with the same purpose of "UpdateDOMData()";
**  08-Apr-2004 (schph01)
**     bug #112128 Add missing case OT_INDEX in UpdateDOMDataDetail1()
**     function.
********************************************************************/

#include "libmon.h"
#include "domdloca.h"
#include "error.h"

static BOOL bIsNodesListInSpecialState = FALSE;
extern BOOL isNodesListInSpecialState()
{
	 return bIsNodesListInSpecialState;
}

static BOOL bRetryReplicSvrRefresh = FALSE;
void LIBMON_RetrySvrStatusRefresh(BOOL bRetry)
{
	bRetryReplicSvrRefresh = bRetry;
}



int GetRepObjectType4ll(iobjecttype,ireplicver)
int iobjecttype;
int ireplicver;
{
   switch (ireplicver) {
      case REPLIC_NOINSTALL:
         myerror(ERR_REPLICTYPE);
         return iobjecttype;
         break;
      case REPLIC_V10:
      case REPLIC_V105:
         return iobjecttype;
         break;
      case REPLIC_V11:
         {
            switch (iobjecttype) {
               case OT_REPLIC_CONNECTION:
                  return OT_REPLIC_CONNECTION_V11;
                  break;
               case OT_REPLIC_REGTABLE:
                  return OT_REPLIC_REGTABLE_V11;
                  break;
               case OT_REPLIC_MAILUSER:
                  return OT_REPLIC_MAILUSER_V11;
                  break;
               case OT_REPLIC_CDDS:
                  return OT_REPLIC_CDDS_V11;
                  break;
               case OTLL_REPLIC_CDDS:
                  return OTLL_REPLIC_CDDS_V11;
                  break;
               case OT_REPLIC_CDDS_DETAIL:
                  return OT_REPLIC_CDDS_DETAIL_V11;
                  break;
               case OT_REPLIC_DISTRIBUTION:
                  return OT_REPLIC_DISTRIBUTION_V11;
                  break;
               case OT_REPLIC_REGTBLS:
                  return OT_REPLIC_REGTBLS_V11;
                  break;
               case OT_REPLIC_REGCOLS:
                  return OT_REPLIC_REGCOLS_V11;
                  break;
            }
            myerror(ERR_REPLICTYPE);
            return iobjecttype;
         }
         break;
   }
   myerror(ERR_REPLICTYPE);
   return iobjecttype;
}


static int UpdateDOMDataDetail1(hnodestruct,iobjecttype,level,parentstrings,
                        bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
BOOL    bWithChildren;
BOOL    bOnlyIfExist;
BOOL    bUnique;
int		thdl;
{
         
   int   i, iold, iobj, iobj2, iret, iret1, icmp;
   struct  ServConnectData * pdata1;
   UCHAR   buf[MAXOBJECTNAME];  
   UCHAR   buffilter[MAXOBJECTNAME];
   UCHAR   extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   UCHAR   bufparent1WithOwner[MAXOBJECTNAME];
   void *  ptemp;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   pdata1 = virtnode[hnodestruct].pdata;

   iret = RES_SUCCESS;

   switch (iobjecttype) {
      case OT_MON_ALL  :
         bWithChildren=TRUE;
         bOnlyIfExist=TRUE;
		 bUnique = FALSE;
		 bWithSystem = TRUE;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_DATABASE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            // OT_MON_LOCKLIST needs to be first because it's sub-branches (LOCKS) are used for Server's subbranches
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_LOCKLIST,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_SERVER, 0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OTLL_MON_RESOURCE, 0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_LOGPROCESS,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_LOGDATABASE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_TRANSACTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_REPLIC_SERVER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            // notification here:
            SetEvent(hgEventDataChange);
         break;

// monitoring
      case OT_MON_SERVER :
         {
            struct ServerData *pServerdatanew, *pServerdatanew0, *pServerdataold;
            struct ServerData *ptempold;
            if (bUnique && pdata1->ServerRefreshParms.LastDOMUpdver==DOMUpdver)
               goto Serverchildren;

            if (bOnlyIfExist && !pdata1->lpServerData)
               break;

            pServerdataold  = pdata1->lpServerData;
            pServerdatanew0 = (struct ServerData * )
                                      ESL_AllocMem(sizeof(struct ServerData));
            if (!pServerdatanew0)
               return RES_ERR;

            pServerdatanew = pServerdatanew0;
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pServerdatanew->ServerDta),
                                       NULL,extradata, thdl);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpServerData  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbservers=0;
                        pdata1->serverserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpServerData)
                        pdata1->lpServerData = pServerdatanew0;
                     else 
                        freeServerData(pServerdatanew0);
                     goto Serverchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pServerdataold && iold<pdata1->nbservers) {
                  icmp = LIBMON_CompareMonInfo(OT_MON_SERVER,
                                        &pServerdatanew->ServerDta,
                                        &pServerdataold->ServerDta, thdl);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pServerdatanew->ptemp = pServerdataold;
                     pServerdataold=GetNextServer(pServerdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pServerdataold=GetNextServer(pServerdataold);
               }
               pServerdatanew=GetNextServer(pServerdatanew);
               if (!pServerdatanew) {
                  iret1 = RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pServerdatanew->ServerDta),buffilter,extradata, thdl);
            }
            pdata1->nbservers = i;
            for (i=0,pServerdatanew=pServerdatanew0; i<pdata1->nbservers;
                            i++,pServerdatanew=GetNextServer(pServerdatanew)) {
               if (pServerdatanew->ptemp) {
                  ptemp    = (void *) pServerdatanew->pnext;
                  ptempold =          pServerdatanew->ptemp;
                  ptempold->ServerDta = pServerdatanew->ServerDta;
                  memcpy(pServerdatanew,ptempold,sizeof (struct ServerData));
                  pServerdatanew->pnext = (struct ServerData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct ServerData));
                  ptempold->pnext = (struct ServerData *) ptemp;
               }
            }
            freeServerData(pdata1->lpServerData);
            pdata1->lpServerData = pServerdatanew0;
            pdata1->serverserrcode=RES_SUCCESS;
Serverchildren:
            UpdRefreshParams(&(pdata1->ServerRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all Servers */
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_SESSION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_CONNECTED_USER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_ACTIVE_USER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_TRANSACTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_CURSOR,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_FILEINFO,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_ICE_DB_CONNECTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
            }
         }
         break;
      case OT_MON_LOCKLIST :
         {
            struct LockListData *pLockListdatanew, *pLockListdatanew0, *pLockListdataold;
            struct LockListData *ptempold;
            if (bUnique && pdata1->LockListRefreshParms.LastDOMUpdver==DOMUpdver)
               goto LockListchildren;

            if (bOnlyIfExist && !pdata1->lpLockListData)
               break;

            pLockListdataold  = pdata1->lpLockListData;
            pLockListdatanew0 = (struct LockListData * )
                                      ESL_AllocMem(sizeof(struct LockListData));
            if (!pLockListdatanew0)
               return RES_ERR;

            pLockListdatanew = pLockListdatanew0;
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLockListdatanew->LockListDta),
                                       NULL,extradata, thdl);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpLockListData  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nblocklists=0;
                        pdata1->LockListserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpLockListData)
                        pdata1->lpLockListData = pLockListdatanew0;
                     else 
                        freeLockListData(pLockListdatanew0);
                     goto LockListchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pLockListdataold && iold<pdata1->nblocklists) {
                  icmp = LIBMON_CompareMonInfo(OT_MON_LOCKLIST,
                                        &pLockListdatanew->LockListDta,
                                        &pLockListdataold->LockListDta, thdl);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pLockListdatanew->ptemp = pLockListdataold;
                     pLockListdataold=GetNextLockList(pLockListdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pLockListdataold=GetNextLockList(pLockListdataold);
               }
               pLockListdatanew=GetNextLockList(pLockListdatanew);
               if (!pLockListdatanew) {
                  iret1 = RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLockListdatanew->LockListDta),buffilter,extradata,thdl);
               if (iret1==RES_ENDOFDATA) {
                  pLockListdatanew->LockListDta.bIs4AllLockLists = TRUE;
                  i++;
               }
            }
            pdata1->nblocklists = i;
            for (i=0,pLockListdatanew=pLockListdatanew0; i<pdata1->nblocklists;
                            i++,pLockListdatanew=GetNextLockList(pLockListdatanew)) {
               if (pLockListdatanew->ptemp) {
                  ptemp    = (void *) pLockListdatanew->pnext;
                  ptempold =          pLockListdatanew->ptemp;
                  ptempold->LockListDta = pLockListdatanew->LockListDta;
                  memcpy(pLockListdatanew,ptempold,sizeof (struct LockListData));
                  pLockListdatanew->pnext = (struct LockListData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct LockListData));
                  ptempold->pnext = (struct LockListData *) ptemp;
               }
            }
            freeLockListData(pdata1->lpLockListData);
            pdata1->lpLockListData = pLockListdatanew0;
            pdata1->LockListserrcode=RES_SUCCESS;
LockListchildren:
            UpdRefreshParams(&(pdata1->LockListRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all LockLists */
              iret1 = UpdateDOMDataDetail1(hnodestruct,OT_MON_LOCKLIST_LOCK,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
            }
         }
         break;
      case OTLL_MON_RESOURCE :
         {
            struct ResourceData *pResourcedatanew, *pResourcedatanew0, *pResourcedataold;
            struct ResourceData *ptempold;
            if (bUnique && pdata1->ResourceRefreshParms.LastDOMUpdver==DOMUpdver)
               goto Resourcechildren;

            if (bOnlyIfExist && !pdata1->lpResourceData)
               break;

            pResourcedataold  = pdata1->lpResourceData;
            pResourcedatanew0 = (struct ResourceData * )
                                      ESL_AllocMem(sizeof(struct ResourceData));
            if (!pResourcedatanew0)
               return RES_ERR;

            pResourcedatanew = pResourcedatanew0;
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pResourcedatanew->ResourceDta),
                                       NULL,extradata, thdl);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpResourceData  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbresources=0;
                        pdata1->resourceserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpResourceData)
                        pdata1->lpResourceData = pResourcedatanew0;
                     else 
                        freeResourceData(pResourcedatanew0);
                     goto Resourcechildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pResourcedataold && iold<pdata1->nbresources) {
                  icmp = LIBMON_CompareMonInfo(OTLL_MON_RESOURCE,
                                       &pResourcedatanew->ResourceDta,
                                       &pResourcedataold->ResourceDta, thdl);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pResourcedatanew->ptemp = pResourcedataold;
                     pResourcedataold=GetNextResource(pResourcedataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pResourcedataold=GetNextResource(pResourcedataold);
               }
               pResourcedatanew=GetNextResource(pResourcedatanew);
               if (!pResourcedatanew) {
                  iret1 = RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pResourcedatanew->ResourceDta),buffilter,extradata,thdl);
            }
            pdata1->nbresources = i;
            for (i=0,pResourcedatanew=pResourcedatanew0; i<pdata1->nbresources;
                            i++,pResourcedatanew=GetNextResource(pResourcedatanew)) {
               if (pResourcedatanew->ptemp) {
                  ptemp    = (void *) pResourcedatanew->pnext;
                  ptempold =          pResourcedatanew->ptemp;
                  ptempold->ResourceDta = pResourcedatanew->ResourceDta;
                  memcpy(pResourcedatanew,ptempold,sizeof (struct ResourceData));
                  pResourcedatanew->pnext = (struct ResourceData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct ResourceData));
                  ptempold->pnext = (struct ResourceData *) ptemp;
               }
            }
            freeResourceData(pdata1->lpResourceData);
            pdata1->lpResourceData = pResourcedatanew0;
            pdata1->resourceserrcode=RES_SUCCESS;
Resourcechildren:
            UpdRefreshParams(&(pdata1->ResourceRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
//              aparents[0]=NULL; /* for all Resources */
//              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_RES_LOCK,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique);
//              if (iret1 != RES_SUCCESS)
//                  iret=iret1;
            }
         }
         break;
      case OT_MON_LOGPROCESS  :
         {
            struct LogProcessData * pLogProcessdatanew, *pLogProcessdatanew0;

            if (bOnlyIfExist && !pdata1->lpLogProcessData)
               break;

            if (bUnique && pdata1->LogProcessesRefreshParms.LastDOMUpdver==DOMUpdver)
               goto LogProcesschildren;

            pLogProcessdatanew0 = (struct LogProcessData * )
                                 ESL_AllocMem(sizeof(struct LogProcessData));
            if (!pLogProcessdatanew0) 
               return RES_ERR;

            pLogProcessdatanew = pLogProcessdatanew0; 
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogProcessdatanew->LogProcessDta),
                                       NULL,extradata, thdl);

            for (i=0;iret1!=RES_ENDOFDATA;i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpLogProcessData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbLogProcesses=0;
                        pdata1->logprocesseserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpLogProcessData)
                        pdata1->lpLogProcessData = pLogProcessdatanew0;
                     else 
                        freeLogProcessData(pLogProcessdatanew0);
                     goto LogProcesschildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               pLogProcessdatanew= GetNextLogProcess(pLogProcessdatanew);
               if (!pLogProcessdatanew) {
                  iret1=RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLogProcessdatanew->LogProcessDta),buffilter,extradata,thdl);
            }
            pdata1->nbLogProcesses = i;
            freeLogProcessData(pdata1->lpLogProcessData);
            pdata1->lpLogProcessData = pLogProcessdatanew0;
            pdata1->logprocesseserrcode=RES_SUCCESS;
LogProcesschildren:
            UpdRefreshParams(&(pdata1->LogProcessesRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
             /* no children */
            }
         }
         break;

      case OT_MON_LOGDATABASE  :
         {
            struct LogDBData * pLogDBdatanew, *pLogDBdatanew0;

            if (bOnlyIfExist && !pdata1->lpLogDBData)
               break;

            if (bUnique && pdata1->LogDBRefreshParms.LastDOMUpdver==DOMUpdver)
               goto LogDBchildren;

            pLogDBdatanew0 = (struct LogDBData * )
                                 ESL_AllocMem(sizeof(struct LogDBData));
            if (!pLogDBdatanew0) 
               return RES_ERR;

            pLogDBdatanew = pLogDBdatanew0; 
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogDBdatanew->LogDBDta),
                                       NULL,extradata, thdl);

            for (i=0;iret1!=RES_ENDOFDATA;i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpLogDBData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbLogDB=0;
                        pdata1->logDBerrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpLogDBData)
                        pdata1->lpLogDBData = pLogDBdatanew0;
                     else 
                        freeLogDBData(pLogDBdatanew0);
                     goto LogDBchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               pLogDBdatanew= GetNextLogDB(pLogDBdatanew);
               if (!pLogDBdatanew) {
                  iret1=RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLogDBdatanew->LogDBDta),buffilter,extradata,thdl);
            }
            pdata1->nbLogDB = i;
            freeLogDBData(pdata1->lpLogDBData);
            pdata1->lpLogDBData = pLogDBdatanew0;
            pdata1->logDBerrcode=RES_SUCCESS;
LogDBchildren:
            UpdRefreshParams(&(pdata1->LogDBRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
             /* no children */
            }
         }
         break;
      case OT_MON_TRANSACTION  :
         {
            struct LogTransactData * pLogTransactdatanew, *pLogTransactdatanew0;

            if (bOnlyIfExist && !pdata1->lpLogTransactData)
               break;

            if (bUnique && pdata1->LogTransactRefreshParms.LastDOMUpdver==DOMUpdver)
               goto LogTransactchildren;

            pLogTransactdatanew0 = (struct LogTransactData * )
                                 ESL_AllocMem(sizeof(struct LogTransactData));
            if (!pLogTransactdatanew0) 
               return RES_ERR;

            pLogTransactdatanew = pLogTransactdatanew0; 
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogTransactdatanew->LogTransactDta),
                                       NULL,extradata, thdl);

            for (i=0;iret1!=RES_ENDOFDATA;i++) {
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpLogTransactData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbLogTransact=0;
                        pdata1->LogTransacterrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpLogTransactData)
                        pdata1->lpLogTransactData = pLogTransactdatanew0;
                     else 
                        freeLogTransactData(pLogTransactdatanew0);
                     goto LogTransactchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               pLogTransactdatanew= GetNextLogTransact(pLogTransactdatanew);
               if (!pLogTransactdatanew) {
                  iret1=RES_ERR;
                  continue;
               }
               iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLogTransactdatanew->LogTransactDta),buffilter,extradata,thdl);
            }
            pdata1->nbLogTransact = i;
            freeLogTransactData(pdata1->lpLogTransactData);
            pdata1->lpLogTransactData = pLogTransactdatanew0;
            pdata1->LogTransacterrcode=RES_SUCCESS;
LogTransactchildren:
            UpdRefreshParams(&(pdata1->LogTransactRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
             /* no children */
            }
         }
         break;
      case OT_MON_SESSION :
         {
            struct ServerData     * pServerdata;
            struct SessionData * pSessiondatanew, * pSessiondatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpServerSessionData)
                  continue;

               if (bUnique && pServerdata->ServerSessionsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto Sessionchildren;

               pSessiondatanew0 = (struct SessionData * )
                                  ESL_AllocMem(sizeof(struct SessionData));
               if (!pSessiondatanew0) 
                  return RES_ERR;

               pSessiondatanew = pSessiondatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pSessiondatanew->SessionDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpServerSessionData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbServerSessions=0;
                           pServerdata->ServerSessionserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpServerSessionData)
                           pServerdata->lpServerSessionData = pSessiondatanew0;
                        else 
                           freeSessionData(pSessiondatanew0);
                        goto Sessionchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pSessiondatanew=GetNextSession(pSessiondatanew);
                  if (!pSessiondatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pSessiondatanew->SessionDta),NULL,extradata,thdl);
               }
               pServerdata->nbServerSessions = i;
               freeSessionData(pServerdata->lpServerSessionData);
               pServerdata->lpServerSessionData = pSessiondatanew0;
               pServerdata->ServerSessionserrcode=RES_SUCCESS;
Sessionchildren:
               UpdRefreshParams(&(pServerdata->ServerSessionsRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;
      case OT_MON_ICE_CONNECTED_USER :
         {
            struct ServerData     * pServerdata;
            struct IceConnUserData * pIceConnuserdatanew, * pIceConnuserdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceConnUsersData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceConnUsersRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceConnUserchildren;

               pIceConnuserdatanew0 = (struct IceConnUserData * )
                                  ESL_AllocMem(sizeof(struct IceConnUserData));
               if (!pIceConnuserdatanew0) 
                  return RES_ERR;

               pIceConnuserdatanew = pIceConnuserdatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceConnuserdatanew->IceConnUserDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceConnUsersData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbiceconnusers=0;
                           pServerdata->iceConnuserserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceConnUsersData)
                           pServerdata->lpIceConnUsersData = pIceConnuserdatanew;
                        else 
                           freeIceConnectUsersData(pIceConnuserdatanew);
                        goto IceConnUserchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIceConnuserdatanew=GetNextIceConnUser(pIceConnuserdatanew);
                  if (!pIceConnuserdatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIceConnuserdatanew->IceConnUserDta),NULL,extradata,thdl);
               }
               pServerdata->nbiceconnusers = i;
               freeIceConnectUsersData(pServerdata->lpIceConnUsersData);
               pServerdata->lpIceConnUsersData = pIceConnuserdatanew0;
               pServerdata->iceConnuserserrcode=RES_SUCCESS;
IceConnUserchildren:
               UpdRefreshParams(&(pServerdata->IceConnUsersRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_ACTIVE_USER :
         {
            struct ServerData     * pServerdata;
            struct IceActiveUserData * pIceActiveuserdatanew, * pIceActiveuserdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceActiveUsersData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceActiveUsersRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceActiveUserchildren;

               pIceActiveuserdatanew0 = (struct IceActiveUserData * )
                                  ESL_AllocMem(sizeof(struct IceActiveUserData));
               if (!pIceActiveuserdatanew0) 
                  return RES_ERR;

               pIceActiveuserdatanew = pIceActiveuserdatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceActiveuserdatanew->IceActiveUserDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceActiveUsersData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbiceactiveusers=0;
                           pServerdata->iceactiveuserserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceActiveUsersData)
                           pServerdata->lpIceActiveUsersData = pIceActiveuserdatanew;
                        else 
                           freeIceActiveUsersData(pIceActiveuserdatanew);
                        goto IceActiveUserchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIceActiveuserdatanew=GetNextIceActiveUser(pIceActiveuserdatanew);
                  if (!pIceActiveuserdatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIceActiveuserdatanew->IceActiveUserDta),NULL,extradata,thdl);
               }
               pServerdata->nbiceactiveusers = i;
               freeIceActiveUsersData(pServerdata->lpIceActiveUsersData);
               pServerdata->lpIceActiveUsersData = pIceActiveuserdatanew0;
               pServerdata->iceactiveuserserrcode=RES_SUCCESS;
IceActiveUserchildren:
               UpdRefreshParams(&(pServerdata->IceActiveUsersRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_TRANSACTION :
         {
            struct ServerData     * pServerdata;
            struct IceTransactData * pIcetransactdatanew, * pIcetransactdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceTransactionsData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceTransactionsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceTransactchildren;

               pIcetransactdatanew0 = (struct IceTransactData * )
                                  ESL_AllocMem(sizeof(struct IceTransactData));
               if (!pIcetransactdatanew0) 
                  return RES_ERR;

               pIcetransactdatanew = pIcetransactdatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIcetransactdatanew->IceTransactDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceTransactionsData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbicetransactions=0;
                           pServerdata->icetransactionserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceTransactionsData)
                           pServerdata->lpIceTransactionsData = pIcetransactdatanew;
                        else 
                           freeIcetransactionsData(pIcetransactdatanew);
                        goto IceTransactchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIcetransactdatanew=GetNextIceTransaction(pIcetransactdatanew);
                  if (!pIcetransactdatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIcetransactdatanew->IceTransactDta),NULL,extradata,thdl);
               }
               pServerdata->nbicetransactions = i;
               freeIcetransactionsData(pServerdata->lpIceTransactionsData);
               pServerdata->lpIceTransactionsData = pIcetransactdatanew0;
               pServerdata->icetransactionserrcode=RES_SUCCESS;
IceTransactchildren:
               UpdRefreshParams(&(pServerdata->IceTransactionsRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_CURSOR :
         {
            struct ServerData     * pServerdata;
            struct IceCursorData * pIceCursordatanew, * pIceCursordatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }

               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceCursorsData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceCursorsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceCursorchildren;

               pIceCursordatanew0 = (struct IceCursorData * )
                                  ESL_AllocMem(sizeof(struct IceCursorData));
               if (!pIceCursordatanew0) 
                  return RES_ERR;

               pIceCursordatanew = pIceCursordatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceCursordatanew->IceCursorDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceCursorsData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbicecursors=0;
                           pServerdata->icecursorsserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceCursorsData)
                           pServerdata->lpIceCursorsData = pIceCursordatanew;
                        else 
                           freeIceCursorsData(pIceCursordatanew);
                        goto IceCursorchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIceCursordatanew=GetNextIceCursor(pIceCursordatanew);
                  if (!pIceCursordatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIceCursordatanew->IceCursorDta),NULL,extradata,thdl);
               }
               pServerdata->nbicecursors = i;
               freeIceCursorsData(pServerdata->lpIceCursorsData);
               pServerdata->lpIceCursorsData = pIceCursordatanew0;
               pServerdata->icecursorsserrcode=RES_SUCCESS;
IceCursorchildren:
               UpdRefreshParams(&(pServerdata->IceCursorsRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_FILEINFO :
         {
            struct ServerData     * pServerdata;
            struct IceCacheData * pIceCachedatanew, * pIceCachedatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }

               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceCacheInfoData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceCacheRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceCachechildren;

               pIceCachedatanew0 = (struct IceCacheData * )
                                  ESL_AllocMem(sizeof(struct IceCacheData));
               if (!pIceCachedatanew0) 
                  return RES_ERR;

               pIceCachedatanew = pIceCachedatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceCachedatanew->IceCacheDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceCacheInfoData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbicecacheinfo=0;
                           pServerdata->icecacheinfoerrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceCacheInfoData)
                           pServerdata->lpIceCacheInfoData = pIceCachedatanew;
                        else 
                           freeIceCacheData(pIceCachedatanew);
                        goto IceCachechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIceCachedatanew=GetNextIceCacheInfo(pIceCachedatanew);
                  if (!pIceCachedatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIceCachedatanew->IceCacheDta),NULL,extradata,thdl);
               }
               pServerdata->nbicecacheinfo = i;
               freeIceCacheData(pServerdata->lpIceCacheInfoData);
               pServerdata->lpIceCacheInfoData = pIceCachedatanew0;
               pServerdata->icecacheinfoerrcode=RES_SUCCESS;
IceCachechildren:
               UpdRefreshParams(&(pServerdata->IceCacheRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_MON_ICE_DB_CONNECTION :
         {
            struct ServerData     * pServerdata;
            struct IceDbConnectData * pIceDbConnectdatanew, * pIceDbConnectdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)  //monitoring:level should be 0 even if parent struct
               return myerror(ERR_LEVELNUMBER);
            pServerdata= pdata1 -> lpServerData;
            if (!pServerdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbservers;
                        iobj++, pServerdata=GetNextServer(pServerdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pServerdata->lpIceDbConnectData)
                  continue;

               if (pServerdata->ServerDta.servertype!=SVR_TYPE_ICE) {
                  myerror(ERR_MONITORTYPE);
                  continue;
               }

               if (bUnique && pServerdata->IceDbConnectRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto IceDBConnectchildren;

               pIceDbConnectdatanew0 = (struct IceDbConnectData * )
                                  ESL_AllocMem(sizeof(struct IceDbConnectData));
               if (!pIceDbConnectdatanew0) 
                  return RES_ERR;

               pIceDbConnectdatanew = pIceDbConnectdatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceDbConnectdatanew->IceDbConnectDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pServerdata->lpIceDbConnectData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pServerdata->nbicedbconnectinfo=0;
                           pServerdata->icedbconnectinfoerrcode=iret1;
                           iret=iret1;
                        }
                        if (!pServerdata->lpIceDbConnectData)
                           pServerdata->lpIceDbConnectData = pIceDbConnectdatanew;
                        else 
                           freeIceDbConnectData(pIceDbConnectdatanew);
                        goto IceDBConnectchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pIceDbConnectdatanew=GetNextIceDbConnectdata(pIceDbConnectdatanew);
                  if (!pIceDbConnectdatanew) {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pIceDbConnectdatanew->IceDbConnectDta),NULL,extradata,thdl);
               }
               pServerdata->nbicedbconnectinfo = i;
               freeIceDbConnectData(pServerdata->lpIceDbConnectData);
               pServerdata->lpIceDbConnectData = pIceDbConnectdatanew0;
               pServerdata->icedbconnectinfoerrcode=RES_SUCCESS;
IceDBConnectchildren:
               UpdRefreshParams(&(pServerdata->IceDbConnectRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;

      case OT_MON_LOCKLIST_LOCK :
         {
            struct LockListData     * pLockListdata;
            struct LockData * pLockdatanew, * pLockdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            if (level!=0)
               return myerror(ERR_LEVELNUMBER);
            pLockListdata= pdata1 -> lpLockListData;
            if (!pLockListdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nblocklists;
                        iobj++, pLockListdata=GetNextLockList(pLockListdata)) {
               if (parentstrings) {
                  if (LIBMON_CompareMonInfo(OT_MON_LOCKLIST, parentstrings,&(pLockListdata->LockListDta), thdl)) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pLockListdata->lpLockData)
                  continue;

               if (bUnique && pLockListdata->LocksRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto Lockchildren;

               pLockdatanew0 = (struct LockData * )
                                  ESL_AllocMem(sizeof(struct LockData));
               if (!pLockdatanew0) 
                  return RES_ERR;

               pLockdatanew = pLockdatanew0;
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *)&pLockListdata->LockListDta,
                                       TRUE,
                                       (LPUCHAR)&(pLockdatanew->LockDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA;i++){
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pLockListdata->lpLockData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pLockListdata->nbLocks=0;
                           pLockListdata->lockserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pLockListdata->lpLockData)
                           pLockListdata->lpLockData = pLockdatanew0;
                        else 
                           freeLockData(pLockdatanew0);
                        goto Lockchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pLockdatanew=GetNextLock(pLockdatanew);
                  if (!pLockdatanew)  {
                     iret1=RES_ERR;
                     continue;
                  }
                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLockdatanew->LockDta),NULL,extradata,thdl);
               }
               pLockListdata->nbLocks = i;
               freeLockData(pLockListdata->lpLockData);
               pLockListdata->lpLockData = pLockdatanew0;
               pLockListdata->lockserrcode=RES_SUCCESS;
Lockchildren:
               UpdRefreshParams(&(pLockListdata->LocksRefreshParms),
                                OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise compile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;
//      case OT_MON_RES_LOCK :
//         {
//            struct ResourceData     * pResourcedata;
//            struct LockData * pLockdatanew, * pLockdatanew0;
//            BOOL bParentFound = parentstrings? FALSE: TRUE;
//            if (level!=1)
//               return myerror(ERR_LEVELNUMBER);
//            pResourcedata= pdata1 -> lpResourceData;
//            if (!pResourcedata) {
//               if (bOnlyIfExist)
//                 break;
//               return myerror(ERR_UPDATEDOMDATA1);
//            }
//            for (iobj=0;iobj<pdata1->nbresources;
//                        iobj++, pResourcedata=GetNextResource(pResourcedata)) {
//               if (parentstrings[0]) {
//                  if (x_strcmp(parentstrings[0],pResourcedata->ResourceDta.ResourceName))
//                     continue;
//               }
//               bParentFound = TRUE;
//
//               if (bOnlyIfExist && !pResourcedata->lpResourceLockData)
//                  continue;
//
//               if (bUnique && pResourcedata->ResourceLocksRefreshParms.LastDOMUpdver==DOMUpdver)
//                  goto ResourceLockchildren;
//
//               pLockdatanew0 = (struct LockData * )
//                                  ESL_AllocMem(sizeof(struct LockData));
//               if (!pLockdatanew0) 
//                  return RES_ERR;
//
//               pLockdatanew = pLockdatanew0;
//               aparents[0]=pResourcedata->ResourceDta.ResourceName; /* for current Resource */
//               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
//                                       iobjecttype,
//                                       1,
//                                       aparents,
//                                       TRUE,
//                                       (LPUCHAR)&(pLockdatanew->LockDta),
//                                       NULL,extradata);
//               for (i=0;iret1!=RES_ENDOFDATA; i++){
//                  switch (iret1) {
//                     case RES_ERR:
//                     case RES_NOGRANT:
//                     case RES_TIMEOUT:
//                        if(!(pResourcedata->lpResourceLockData &&
//                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
//                           pResourcedata->nbResourceLocks=0;
//                           pResourcedata->ResourceLockserrcode=iret1;
//                           iret=iret1;
//                        }
//                        if (!pResourcedata->lpResourceLockData)
//                           pResourcedata->lpResourceLockData = pLockdatanew0;
//                        else 
//                           freeLockData(pLockdatanew0);
//                        goto ResourceLockchildren;
//                        break;
//                     case RES_ENDOFDATA:
//                        continue;
//                        break;
//                  }
//                  pLockdatanew = GetNextLock(pLockdatanew);
//                  if (!pLockdatanew)  {
//                     iret1=RES_ERR;
//                     continue;
//                  }
//                  iret1 = LIBMON_DBAGetNextObject((LPUCHAR)&(pLockdatanew->LockDta),NULL,extradata);
//               }
//               pResourcedata->nbResourceLocks = i;
//               freeLockData(pResourcedata->lpResourceLockData);
//               pResourcedata->lpResourceLockData = pLockdatanew0;
//               pResourcedata->ResourceLockserrcode=RES_SUCCESS;
//ResourceLockchildren:
//               UpdRefreshParams(&(pResourcedata->ResourceLocksRefreshParms),
//                                OT_MON_RES_LOCK);
//               /* no children */
//               iret=iret; /* otherwise compile error */
//            }
//            if (!bParentFound)
//               myerror(ERR_PARENTNOEXIST);
//
//         }
//         break;
//
// end of monitoring


      case OT_DATABASE  :
         {
            struct DBData * pDBdatanew,* pDBdatanew0, *pDBdataold, *ptempold;
            pDBdataold  = pdata1 -> lpDBData;
            if (bOnlyIfExist && !pDBdataold)
               break;

            if (bUnique && pdata1->DBRefreshParms.LastDOMUpdver==DOMUpdver) 
               goto DBchildren;

            pDBdatanew0 = (struct DBData *)
                               ESL_AllocMem(sizeof(struct DBData));
            if (!pDBdatanew0) 
               return RES_ERR;

            pDBdatanew  = pDBdatanew0;
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       buffilter,extradata, thdl);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++,pDBdatanew=GetNextDB(pDBdatanew)) {
               if (!pDBdatanew)
                  iret1 = RES_ERR;
               /* pDBdatanew points to zeroed area as the list is new */
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!( pdata1 -> lpDBData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbdb=0;
                        pdata1->DBerrcode=iret1;
                        iret = iret1;
                     }
                     if (!pdata1 -> lpDBData)
                       pdata1 -> lpDBData = pDBdatanew0;
                     else 
                       freeDBData(pDBdatanew0);
                     goto DBchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pDBdataold && iold<pdata1->nbdb) {
                  icmp = x_strcmp(buf,pDBdataold->DBName);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pDBdatanew->ptemp = pDBdataold;
                     pDBdataold=GetNextDB(pDBdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pDBdataold=GetNextDB(pDBdataold);
               }
               fstrncpy(pDBdatanew->DBName,buf, MAXOBJECTNAME);
               fstrncpy(pDBdatanew->DBOwner,buffilter,MAXOBJECTNAME);
               pDBdatanew->DBid = getint(extradata);
               pDBdatanew->DBType = getint(extradata+ STEPSMALLOBJ);
               x_strcpy(pDBdatanew->CDBName,extradata+2*STEPSMALLOBJ);
               iret1 =LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
            }
            pdata1->nbdb = i;
            for (i=0,pDBdatanew=pDBdatanew0; i<pdata1->nbdb;
                            i++,pDBdatanew=GetNextDB(pDBdatanew)) {
               if (pDBdatanew->ptemp) {
                  ptemp   = (void *) pDBdatanew->pnext;
                  ptempold=          pDBdatanew->ptemp;
                  /* pDBdatanew->DBName is the same in both structures */
                  fstrncpy(buffilter,pDBdatanew->DBOwner,MAXOBJECTNAME);
                  memcpy(pDBdatanew,ptempold,sizeof (struct DBData));
                  pDBdatanew->pnext = (struct DBData *) ptemp;
                  fstrncpy(pDBdatanew->DBOwner,buffilter,MAXOBJECTNAME);
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct DBData));
                  ptempold->pnext = (struct DBData *) ptemp;
               }
            }
            freeDBData(pdata1->lpDBData);
            pdata1->lpDBData = pDBdatanew0;
            pdata1->DBerrcode=RES_SUCCESS;
DBchildren:
            UpdRefreshParams(&(pdata1->DBRefreshParms),OT_DATABASE);
            if (bWithChildren) {
               int itype[]={OT_TABLE,
                            OT_DBEVENT};
               aparents[0]=NULL; /* for all databases */
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  iret1 = UpdateDOMDataDetail1(hnodestruct,itype[i],1,aparents,
                                        bWithSystem,bWithChildren,bOnlyIfExist,bUnique, thdl);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
               }
            }
         }
         break;



      case OT_USER      :
         {
            struct UserData * puserdatanew, * puserdatanew0;

            // for fix of issue 92465: moved test to beginning of case and tested pdata1
            if (bOnlyIfExist) {
              if (!pdata1)
                break;
              if (!pdata1->lpUsersData)
               break;
            }

            if (bUnique && pdata1->UsersRefreshParms.LastDOMUpdver==DOMUpdver) 
               goto userchildren;

            puserdatanew0 = (struct UserData * )
                              ESL_AllocMem(sizeof(struct UserData));
            if (!puserdatanew0) 
               return RES_ERR;

            puserdatanew = puserdatanew0;
            iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata, thdl);

            for (i=0;iret1!=RES_ENDOFDATA;
                 i++,puserdatanew=GetNextUser(puserdatanew)) {
               if (!puserdatanew)
                  iret1 = RES_ERR;
               /* puserdatanew points to zeroed area as the list is new */
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!( pdata1->lpUsersData &&
                         AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbusers=0;
                        pdata1->userserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpUsersData)
                        pdata1->lpUsersData = puserdatanew0;
                     else 
                        freeUsersData(puserdatanew0);
                     goto userchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               fstrncpy(puserdatanew->UserName,buf, MAXOBJECTNAME);

			   if (extradata[0]==ATTACHED_YES)
				   puserdatanew->bAuditAll=TRUE;
			   else
				   puserdatanew->bAuditAll=FALSE;

               iret1 = LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
            }
            pdata1->nbusers = i;
            freeUsersData(pdata1->lpUsersData);
            pdata1->lpUsersData = puserdatanew0;
            pdata1->userserrcode=RES_SUCCESS;
userchildren:
            UpdRefreshParams(&(pdata1->UsersRefreshParms),OT_USER);
            if (bWithChildren) {
             /* no children */
            }
         }
         break;

      case OT_TABLE     :
         {
            struct DBData    * pDBdata;
            struct TableData * ptabledatanew, *ptabledatanew0, *ptabledataold;
            struct TableData * ptempold;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound   = TRUE;
               ptabledataold  = pDBdata->lpTableData;

               if (bOnlyIfExist && !ptabledataold)
                  continue;

               if (bUnique && pDBdata->TablesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto tablechildren;

               ptabledatanew0 = (struct TableData * )
                                 ESL_AllocMem(sizeof(struct TableData));
               if (!ptabledatanew0) 
                    return RES_ERR;
               ptabledatanew = ptabledatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata, thdl);
               for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                        i++,ptabledatanew=GetNextTable(ptabledatanew)) {
                  if (!ptabledatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpTableData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbtables=0;
                           pDBdata->tableserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpTableData)
                           pDBdata->lpTableData= ptabledatanew0;
                        else 
                           freeTablesData(ptabledatanew0);
                        goto tablechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  while (ptabledataold && iold<pDBdata->nbtables) {
                     icmp = DOMTreeCmpTableNames(buf,buffilter,
                           ptabledataold->TableName,ptabledataold->TableOwner);
                     if (icmp<0) 
                        break;
                     if (!icmp) {
                        ptabledatanew->ptemp = ptabledataold;
                        ptabledataold=GetNextTable(ptabledataold);
                                                    /* should be no double*/
                        break;
                     }
                     iold++;
                     ptabledataold=GetNextTable(ptabledataold);
                  }
                  fstrncpy(ptabledatanew->TableName ,buf,      MAXOBJECTNAME);
                  fstrncpy(ptabledatanew->TableOwner,buffilter,MAXOBJECTNAME);
                  ptabledatanew->Tableid=getint(extradata+MAXOBJECTNAME+4);
                  ptabledatanew->TableStarType=getint(extradata+MAXOBJECTNAME+4+STEPSMALLOBJ);
                  iret1 = LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
               }
               pDBdata->nbtables = i;
               for (i=0,ptabledatanew=ptabledatanew0; i<pDBdata->nbtables;
                               i++,ptabledatanew=GetNextTable(ptabledatanew)) {
                  if (ptabledatanew->ptemp) {
                     ptemp   = (void *) ptabledatanew->pnext;
                     ptempold=          ptabledatanew->ptemp;
                     fstrncpy(buf,ptabledatanew->TableName,MAXOBJECTNAME);
                     /* table owner are same in both structures */
                     memcpy(ptabledatanew,ptempold,sizeof (struct TableData));
                     ptabledatanew->pnext = (struct TableData *) ptemp;
                     fstrncpy(ptabledatanew->TableName,buf,MAXOBJECTNAME);
//                     fstrncpy(ptabledatanew->TableOwner,buffilter,MAXOBJECTNAME);
                     ptemp = (void *) ptempold->pnext;
                     memset(ptempold,'\0',sizeof (struct TableData));
                     ptempold->pnext = (struct TableData *) ptemp;
                  }
               }
               freeTablesData(pDBdata->lpTableData);
               pDBdata->lpTableData = ptabledatanew0;
               pDBdata->tableserrcode=RES_SUCCESS;
tablechildren:
               UpdRefreshParams(&(pDBdata->TablesRefreshParms),OT_TABLE);
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_COLUMN :
         {
            struct DBData        * pDBdata;
            struct TableData     * ptabledata;
            struct ColumnData    * pColumndatanew, * pColumndatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bParent2;
            if (level!=2)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata)  {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)){
               if (!pDBdata)
                  return myerror(ERR_LIST);
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound=TRUE;
               ptabledata=pDBdata->lpTableData;
               if (!ptabledata)  {
                  if (bOnlyIfExist)
                     continue;
                  return myerror(ERR_UPDATEDOMDATA1);
               }
               bParent2 = parentstrings[1]? FALSE: TRUE;
               for (iobj2=0;iobj2<pDBdata->nbtables;
                            iobj2++,ptabledata=GetNextTable(ptabledata)) {
                  if (!ptabledata)
                     return myerror(ERR_LIST);
                  if (parentstrings[1]) {
                     if (DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner))
                        continue;
                  }
                  bParent2 = TRUE;

                  if (bOnlyIfExist && !ptabledata->lpColumnData)
                     continue;

                  if (bUnique && ptabledata->ColumnsRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto columnchildren;

                  pColumndatanew0 = (struct ColumnData * )
                                     ESL_AllocMem(sizeof(struct ColumnData));
                  if (!pColumndatanew0) 
                     return RES_ERR;
                  pColumndatanew = pColumndatanew0;
                  aparents[0]=pDBdata->DBName; 
                  aparents[1]=StringWithOwner(ptabledata->TableName,
                                ptabledata->TableOwner,bufparent1WithOwner); 
                  iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata, thdl);
                  for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pColumndatanew=GetNextColumn(pColumndatanew)){
                     if (!pColumndatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(ptabledata->lpColumnData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                parentstrings))){
                              ptabledata->nbcolumns=0;
                              ptabledata->columnserrcode=iret1;
                              iret=iret1;
                           }
                           if (!ptabledata->lpColumnData)
                              ptabledata->lpColumnData = pColumndatanew0;
                           else 
                              freeColumnsData(pColumndatanew0);
                           goto columnchildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pColumndatanew->ColumnName, buf, MAXOBJECTNAME);
                     pColumndatanew->ColumnType =getint(extradata);
                     pColumndatanew->ColumnLen = getint(extradata+STEPSMALLOBJ);
                     pColumndatanew->bWithNull =
                       (*(extradata+2*STEPSMALLOBJ)==ATTACHED_YES)?TRUE:FALSE;
                     pColumndatanew->bWithDef =
                       (*(extradata+2*STEPSMALLOBJ+1)
                        ==ATTACHED_YES)?TRUE:FALSE;
                     iret1 = LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
                  }
                  ptabledata->nbcolumns  =i;
                  freeColumnsData(ptabledata->lpColumnData);
                  ptabledata->lpColumnData = pColumndatanew0;
                  ptabledata->columnserrcode=RES_SUCCESS;
columnchildren:
                  UpdRefreshParams(&(ptabledata->ColumnsRefreshParms),
                                   OT_COLUMN);
                  /* no children */
                  iret=iret;
               }
               if (!bParent2) 
                  return myerror(ERR_PARENTNOEXIST);
            }
            if (!bParentFound) 
               return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_VIEWCOLUMN :
         {
            struct DBData        * pDBdata;
            struct ViewData      * pViewdata;
            struct ColumnData    * pColumndatanew, * pColumndatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bParent2;
            if (level!=2)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata)  {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)){
               if (!pDBdata)
                  return myerror(ERR_LIST);
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound=TRUE;
               pViewdata=pDBdata->lpViewData;
               if (!pViewdata)  {
                  if (bOnlyIfExist)
                     continue;
                  return myerror(ERR_UPDATEDOMDATA1);
               }
               bParent2 = parentstrings[1]? FALSE: TRUE;
               for (iobj2=0;iobj2<pDBdata->nbviews;
                            iobj2++,pViewdata=GetNextView(pViewdata)) {
                  if (!pViewdata)
                     return myerror(ERR_LIST);
                  if (parentstrings[1]) {
                     if (DOMTreeCmpTableNames(parentstrings[1],"",
                           pViewdata->ViewName,pViewdata->ViewOwner))
                        continue;
                  }
                  bParent2 = TRUE;

                  if (bOnlyIfExist && !pViewdata->lpColumnData)
                     continue;

                  if (bUnique && pViewdata->ColumnsRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto viewcolumnchildren;

                  pColumndatanew0 = (struct ColumnData * )
                                     ESL_AllocMem(sizeof(struct ColumnData));
                  if (!pColumndatanew0) 
                     return RES_ERR;
                  pColumndatanew = pColumndatanew0;
                  aparents[0]=pDBdata->DBName; 
                  aparents[1]=StringWithOwner(pViewdata->ViewName,
                                pViewdata->ViewOwner,bufparent1WithOwner); 
                  iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata, thdl);
                  for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pColumndatanew=GetNextColumn(pColumndatanew)){
                     if (!pColumndatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(pViewdata->lpColumnData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                parentstrings))){
                              pViewdata->nbcolumns=0;
                              pViewdata->columnserrcode=iret1;
                              iret=iret1;
                           }
                           if (!pViewdata->lpColumnData)
                              pViewdata->lpColumnData = pColumndatanew0;
                           else 
                              freeColumnsData(pColumndatanew0);
                           goto viewcolumnchildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pColumndatanew->ColumnName, buf, MAXOBJECTNAME);
                     pColumndatanew->ColumnType =getint(extradata);
                     pColumndatanew->ColumnLen = getint(extradata+STEPSMALLOBJ);
                     pColumndatanew->bWithNull =
                       (*(extradata+2*STEPSMALLOBJ)==ATTACHED_YES)?TRUE:FALSE;
                     pColumndatanew->bWithDef =
                       (*(extradata+2*STEPSMALLOBJ+1)
                        ==ATTACHED_YES)?TRUE:FALSE;
                     iret1 = LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
                  }
                  pViewdata->nbcolumns  =i;
                  freeColumnsData(pViewdata->lpColumnData);
                  pViewdata->lpColumnData = pColumndatanew0;
                  pViewdata->columnserrcode=RES_SUCCESS;
viewcolumnchildren:
                  UpdRefreshParams(&(pViewdata->ColumnsRefreshParms),
                                   OT_COLUMN);
                  /* no children */
                  iret=iret;
               }
               if (!bParent2) 
                  return myerror(ERR_PARENTNOEXIST);
            }
            if (!bParentFound) 
               return myerror(ERR_PARENTNOEXIST);
         }
         break;
#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
      case OT_MON_REPLIC_SERVER       :
         {
            struct DBData            * pDBdata;
            struct MonReplServerData * pmonreplserverdatanew, * pmonreplserverdatanew0;
            struct MonReplServerData * pmonreplserverdatatemp;
			RESOURCEDATAMIN Resdata;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata)  {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)) {
               if (parentstrings) {
				   char buf1[100];
				  if (x_strcmp(LIBMON_GetMonInfoName(hnodestruct, OT_DATABASE, parentstrings, buf1,sizeof(buf1)-1, thdl),
			               pDBdata->DBName)) 
                   continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pDBdata->lpMonReplServerData)
                  continue;

               if (bUnique && pDBdata->MonReplServerRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto MonReplServerChildren;

               pmonreplserverdatanew0 = (struct MonReplServerData * )
                                  ESL_AllocMem(sizeof(struct MonReplServerData));
               if (!pmonreplserverdatanew0) 
                  return RES_ERR;
               pmonreplserverdatanew = pmonreplserverdatanew0;
			   if (parentstrings)
				   Resdata=* (LPRESOURCEDATAMIN)parentstrings;
			   else 
                   FillResStructFromDB(&Resdata,pDBdata->DBName);
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       (LPUCHAR *) &Resdata,
                                       TRUE,
                                       (LPUCHAR) &(pmonreplserverdatanew->MonReplServerDta),
                                       NULL,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA; i++) {
                   switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpMonReplServerData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbmonreplservers=0;
                           pDBdata->monreplservererrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpMonReplServerData)
                           pDBdata->lpMonReplServerData = pmonreplserverdatanew0;
                        else 
                           freeMonReplServerData(pmonreplserverdatanew0);
                        goto MonReplServerChildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
		          for (pmonreplserverdatatemp=pDBdata->lpMonReplServerData,iold=0;
				       pmonreplserverdatatemp && iold<pDBdata->nbmonreplservers;
					   pmonreplserverdatatemp=GetNextMonReplServer(pmonreplserverdatatemp),iold++) {
						   LPREPLICSERVERDATAMIN lpnew=&(pmonreplserverdatanew ->MonReplServerDta);
                           LPREPLICSERVERDATAMIN lpold=&(pmonreplserverdatatemp->MonReplServerDta);
						   if (!LIBMON_CompareMonInfo(OT_MON_REPLIC_SERVER,lpnew,lpold,thdl)) {
							   *lpnew=*lpold;
							   lpold->DBEhdl = DBEHDL_NOTASSIGNED;
						   }
					   }
				  pmonreplserverdatanew=GetNextMonReplServer(pmonreplserverdatanew);
                  if (!pmonreplserverdatanew) {
                     iret1=RES_ERR;
					 continue;
				  }
				  iret1 = LIBMON_DBAGetNextObject((LPUCHAR) &(pmonreplserverdatanew->MonReplServerDta),NULL,extradata,thdl);
               }
               pDBdata->nbmonreplservers = i;
	           for (iold=0,pmonreplserverdatatemp=pmonreplserverdatanew0;iold<i;
			        iold++,pmonreplserverdatatemp=GetNextMonReplServer(pmonreplserverdatatemp)) {
                   LPREPLICSERVERDATAMIN lp1=&(pmonreplserverdatatemp ->MonReplServerDta);
				   int itmp;
			       struct MonReplServerData * pmonreplserverdatatemp1;
		           for (itmp=0,pmonreplserverdatatemp1=pmonreplserverdatanew0;itmp<iold;
			        itmp++,pmonreplserverdatatemp1=GetNextMonReplServer(pmonreplserverdatatemp1)) {
	                   LPREPLICSERVERDATAMIN lp2=&(pmonreplserverdatatemp1 ->MonReplServerDta);
					   if (!x_stricmp(lp1->RunNode,lp2->RunNode) && lp1->serverno==lp2->serverno &&
						   lp1->localdb!=lp2->localdb) {
						    lp1->bMultipleWithDiffLocalDB = TRUE;
							lp2->bMultipleWithDiffLocalDB = TRUE;
					   }
				   }
	
				   if (lp1->DBEhdl==DBEHDL_ERROR) {
					    int i1;
                        if (bRetryReplicSvrRefresh) {
						   push4domg(thdl);
						   i1=DBEReplGetEntry(lp1->LocalDBNode,lp1->LocalDBName, &lp1->DBEhdl, lp1->serverno,NULL);
						   pop4domg(thdl);
						   if (i1==RES_SUCCESS) 
						      lp1->startstatus=REPSVR_UNKNOWNSTATUS;
						   else {
							   lp1->DBEhdl=DBEHDL_ERROR;
							   lp1->startstatus=REPSVR_ERROR;
						   }
						}
						else
							lp1->DBEhdl=DBEHDL_ERROR_ALLWAYS;
				   }
				  
				   if (lp1->DBEhdl==DBEHDL_NOTASSIGNED) {
					    int i1;
						push4domg(thdl);
						i1=DBEReplGetEntry(lp1->LocalDBNode,lp1->LocalDBName, &lp1->DBEhdl, lp1->serverno,NULL);
						pop4domg(thdl);
						if (i1!=RES_SUCCESS) {
							lp1->DBEhdl=DBEHDL_ERROR;
							lp1->startstatus=REPSVR_ERROR;
						}
				   }
				}
               freeMonReplServerData(pDBdata->lpMonReplServerData);
               pDBdata->lpMonReplServerData = pmonreplserverdatanew0;
               pDBdata->monreplservererrcode=RES_SUCCESS;
MonReplServerChildren:
               UpdRefreshParams(&(pDBdata->MonReplServerRefreshParms),
                                 OT_MON_ALL);
               /* no children */
               iret=iret; /* otherwise compile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
#endif

      case OT_DBEVENT                    :
         {
            struct DBData      * pDBdata;
            struct DBEventData * pDBEventdatanew, * pDBEventdatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata)  {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pDBdata->lpDBEventData)
                  continue;

               if (bUnique && pDBdata->DBEventsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto DBEventchildren;

               pDBEventdatanew0 = (struct DBEventData * )
                                   ESL_AllocMem(sizeof(struct DBEventData));
               if (!pDBEventdatanew0) 
                  return RES_ERR;
               pDBEventdatanew = pDBEventdatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = LIBMON_DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata, thdl);
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pDBEventdatanew=GetNextDBEvent(pDBEventdatanew)){
                  if (!pDBEventdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpDBEventData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbDBEvents=0;
                           pDBdata->dbeventserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpDBEventData)
                           pDBdata->lpDBEventData = pDBEventdatanew0;
                        else 
                           freeDBEventsData(pDBEventdatanew0);
                        goto DBEventchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pDBEventdatanew->DBEventName,  buf,
                                                          MAXOBJECTNAME);
                  fstrncpy(pDBEventdatanew->DBEventOwner, buffilter,
                                                          MAXOBJECTNAME);
                  iret1 = LIBMON_DBAGetNextObject(buf,buffilter,extradata,thdl);
               }
               pDBdata->nbDBEvents = i;
               freeDBEventsData(pDBdata->lpDBEventData);
               pDBdata->lpDBEventData = pDBEventdatanew0;
               pDBdata->dbeventserrcode=RES_SUCCESS;
DBEventchildren:
               UpdRefreshParams(&(pDBdata->DBEventsRefreshParms),
                                 OT_DBEVENT);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;

		 case OT_INDEX                      :
         {
            struct DBData        * pDBdata;
            struct TableData     * ptabledata;
            struct IndexData     * pIndexdatanew, * pIndexdatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bParent2;
            if (level!=2)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)){
               if (!pDBdata)
                  return myerror(ERR_LIST);
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound=TRUE;
               ptabledata=pDBdata->lpTableData;
               if (!ptabledata) {
                  if (bOnlyIfExist)
                     continue;
                  return myerror(ERR_UPDATEDOMDATA1);
               }
               bParent2 = parentstrings[1]? FALSE: TRUE;
               for (iobj2=0;iobj2<pDBdata->nbtables;
                            iobj2++,ptabledata=GetNextTable(ptabledata)) {
                  if (!ptabledata)
                     return myerror(ERR_LIST);
                  if (parentstrings[1]) {
                     if (DOMTreeCmpTableNames(parentstrings[1],"",
                           ptabledata->TableName,ptabledata->TableOwner))
                        continue;
                  }
                  bParent2 = TRUE;

                  if (bOnlyIfExist && !ptabledata->lpIndexData)
                     continue;

                  if (bWithChildren &&
                      ptabledata->iIndexDataQuickInfo ==INFOFROMPARENTIMM) {
                     ptabledata->iIndexDataQuickInfo = INFOFROMPARENT;
                     continue;
                  }

                  if (bUnique && ptabledata->IndexesRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto Indexchildren;

                  pIndexdatanew0 = (struct IndexData * )
                                    ESL_AllocMem(sizeof(struct IndexData));
                  if (!pIndexdatanew0) 
                     return RES_ERR;
                  pIndexdatanew = pIndexdatanew0;
                  aparents[0]=pDBdata->DBName; 
                  aparents[1]=StringWithOwner(ptabledata->TableName,
                                ptabledata->TableOwner,bufparent1WithOwner); 
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
                  for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pIndexdatanew=GetNextIndex(pIndexdatanew)){
                     if (!pIndexdatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(ptabledata->lpIndexData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                        parentstrings))){
                              ptabledata->nbIndexes=0;
                              ptabledata->iIndexDataQuickInfo = INFOFROMLL;
                              ptabledata->indexeserrcode=iret1;
                              iret=iret1;
                           }
                           if (!ptabledata->lpIndexData)
                              ptabledata->lpIndexData = pIndexdatanew0;
                           else 
                              freeIndexData(pIndexdatanew0);
                           goto Indexchildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pIndexdatanew->IndexName, buf,
                                                        MAXOBJECTNAME);
                     fstrncpy(pIndexdatanew->IndexOwner,buffilter,
                                                        MAXOBJECTNAME);
                     fstrncpy(pIndexdatanew->IndexStorage,extradata,
                                                        MAXOBJECTNAME);
                     pIndexdatanew->IndexId=getint(extradata+24);
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  ptabledata->nbIndexes  =i;
                  ptabledata->iIndexDataQuickInfo = INFOFROMLL;
                  freeIndexData(ptabledata->lpIndexData);
                  ptabledata->lpIndexData= pIndexdatanew0;
                  ptabledata->indexeserrcode=RES_SUCCESS;
Indexchildren:
                  UpdRefreshParams(&(ptabledata->IndexesRefreshParms),
                                 OT_INDEX);
                  /* no children */
                  iret=iret;
               }
               if (!bParent2) 
                  return myerror(ERR_PARENTNOEXIST);
            }
            if (!bParentFound) 
               return myerror(ERR_PARENTNOEXIST);
         }
         break;

      default:
         {
            return myerror(ERR_OBJECTNOEXISTS);
         }
         break;
   }
   return iret;
}

int UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,bWithChildren)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
BOOL    bWithChildren;
{
	/* 0 is the default thread handle for single thread application */
	return LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,bWithChildren, 0);
}

int LIBMON_UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,bWithChildren, thdl)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
BOOL    bWithChildren;
int		thdl;
{
   return UpdateDOMDataDetail1(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,bWithChildren,FALSE,FALSE,thdl);
}

BOOL UpdateReplStatusinCacheNeedRefresh(LPUCHAR NodeName,LPUCHAR DBName,int svrno,LPUCHAR evttext)
{
   int hdl;
   BOOL b2Refresh=FALSE;

   for (hdl=0;hdl<imaxnodes;hdl++) {
	  struct  ServConnectData * pdata1 = virtnode[hdl].pdata;
      if (pdata1) {
         struct DBData            * pDBdata = pdata1 -> lpDBData;
         struct MonReplServerData * pmonreplserverdata;
		 int idb,isvr;
		 if (!pDBdata)
			continue;
		 for (idb=0;idb<pdata1->nbdb;idb++,pDBdata=GetNextDB(pDBdata)) {
             if (!pDBdata->lpMonReplServerData)
                continue;
             for (pmonreplserverdata=pDBdata->lpMonReplServerData,isvr=0;
		          pmonreplserverdata && isvr<pDBdata->nbmonreplservers;
				  pmonreplserverdata=GetNextMonReplServer(pmonreplserverdata),isvr++) {
			    LPREPLICSERVERDATAMIN lpsvr=&(pmonreplserverdata ->MonReplServerDta);
				if (!x_stricmp(lpsvr->LocalDBNode,NodeName) &&
				    !x_stricmp(lpsvr->LocalDBName,DBName)   &&
				    lpsvr->serverno==svrno){
				   char *lpactive="Active";
                   char *lpstopped="The Server has shutdown";
				   x_strcpy(lpsvr->FullStatus,evttext);
                   lpsvr->startstatus=REPSVR_UNKNOWNSTATUS;
				   if (!x_strnicmp(evttext,lpactive,x_strlen(lpactive))) 
					   lpsvr->startstatus=REPSVR_STARTED;
				   if (!x_strnicmp(evttext,lpstopped,x_strlen(lpstopped)))
                       lpsvr->startstatus=REPSVR_STOPPED;
				   b2Refresh=TRUE;
				}
			}
		 }
      }
   }
   if (b2Refresh)
       SetEvent(hgEventDataChange);
   return b2Refresh;
}

BOOL CleanNodeReplicInfo(int hdl)
{
   struct  ServConnectData * pdata1 = virtnode[hdl].pdata;
   if (pdata1) {
      struct DBData   * pDBdata = pdata1 -> lpDBData;
      int idb;
	  if (!pDBdata)
		  return TRUE;
	  for (idb=0;idb<pdata1->nbdb;idb++,pDBdata=GetNextDB(pDBdata)) {
         if (!pDBdata->lpMonReplServerData)
             continue;
         freeMonReplServerData (pDBdata->lpMonReplServerData);
         pDBdata->lpMonReplServerData = NULL;
      }
   }
   return TRUE;
}

BOOL IsNodeUsed4ExtReplMon(LPUCHAR NodeName)
{
   int hdl;
   char LocalNode[100];
   BOOL bIsLocal=FALSE;
   x_strcpy(LocalNode,LIBMON_getLocalNodeString());
   if (!x_stricmp(NodeName,LocalNode))
	   bIsLocal=TRUE;

   for (hdl=0;hdl<imaxnodes;hdl++) {
	  struct  ServConnectData * pdata1 = virtnode[hdl].pdata;
      if (pdata1) {
         struct DBData            * pDBdata = pdata1 -> lpDBData;
         struct MonReplServerData * pmonreplserverdata;
		 BOOL b2Refresh=FALSE;
		 int idb,isvr;
		 char buf[100];
		 if (!pDBdata)
			 continue;
		 x_strcpy(buf,pdata1->VirtNodeName);
         RemoveGWNameFromString(buf);
         RemoveConnectUserFromString(buf);
		 if (!x_stricmp(NodeName,buf))
			continue;
		 if (bIsLocal && isLocalWithRealNodeName(buf))
			 continue;
		 for (idb=0;idb<pdata1->nbdb;idb++,pDBdata=GetNextDB(pDBdata)) {
             if (!pDBdata->lpMonReplServerData)
                continue;
             for (pmonreplserverdata=pDBdata->lpMonReplServerData,isvr=0;
		          pmonreplserverdata && isvr<pDBdata->nbmonreplservers;
				  pmonreplserverdata=GetNextMonReplServer(pmonreplserverdata),isvr++) {
			    LPREPLICSERVERDATAMIN lpsvr=&(pmonreplserverdata ->MonReplServerDta);
				if (!x_stricmp(lpsvr->LocalDBNode,NodeName) ||
					(bIsLocal && isLocalWithRealNodeName(lpsvr->LocalDBNode))
				   ){
					if (lpsvr->DBEhdl>=0)
                        return TRUE;
				}
			}
		 }
      }
   }
   return FALSE;
}

#ifndef BLD_MULTI /*To make available in Non multi-thread context only */
BOOL CloseCachesExtReplicInfoNeedRefresh(LPUCHAR NodeName)
{
   int hdl;
   char LocalNode[100];
   BOOL bIsLocal=FALSE;
   BOOL b2Refresh=FALSE;
   x_strcpy(LocalNode,LIBMON_getLocalNodeString());
   if (!x_stricmp(NodeName,LocalNode))
	   bIsLocal=TRUE;

   for (hdl=0;hdl<imaxnodes;hdl++) {
	  struct  ServConnectData * pdata1 = virtnode[hdl].pdata;
      if (pdata1) {
         struct DBData            * pDBdata = pdata1 -> lpDBData;
         struct MonReplServerData * pmonreplserverdata;
		 int idb,isvr;
		 char buf[100];
		 if (!pDBdata)
			 continue;
		 x_strcpy(buf,pdata1->VirtNodeName);
         RemoveGWNameFromString(buf);
         RemoveConnectUserFromString(buf);
		 if (!x_stricmp(NodeName,buf))
			continue;
		 if (bIsLocal && isLocalWithRealNodeName(buf))
			 continue;
		 for (idb=0;idb<pdata1->nbdb;idb++,pDBdata=GetNextDB(pDBdata)) {
             if (!pDBdata->lpMonReplServerData)
                continue;
             for (pmonreplserverdata=pDBdata->lpMonReplServerData,isvr=0;
		          pmonreplserverdata && isvr<pDBdata->nbmonreplservers;
				  pmonreplserverdata=GetNextMonReplServer(pmonreplserverdata),isvr++) {
			    LPREPLICSERVERDATAMIN lpsvr=&(pmonreplserverdata ->MonReplServerDta);
				if (!x_stricmp(lpsvr->LocalDBNode,NodeName) ||
					(bIsLocal && isLocalWithRealNodeName(lpsvr->LocalDBNode))
				   ){
					if (lpsvr->DBEhdl>=0) {
                        DBEReplReplReleaseEntry(lpsvr->DBEhdl,NULL);
						lpsvr->DBEhdl     =DBEHDL_ERROR;
						lpsvr->startstatus=REPSVR_UNKNOWNSTATUS;
                        b2Refresh=TRUE;
					}
				}
			}
		 }
      }
   }
   return b2Refresh;
}
#endif

