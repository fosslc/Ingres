/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**
**  Project : Visual DBA
**
**    Author : Francois Noirot-Nerin
**
**    Source : domdupdt.c
**    Update of cache from low-level
**
**    History after 04-May-1999:
**
**    27-Sep-1999 (schph01)
**      bug #98837 : Add case OTR_CDB in function UpdateDOMDataDetail()
**    28-01-2000 (noifr01)
**     (bug 100196) when VDBA is invoked in the context, enforce a refresh
**     of the nodes list in the cache when the list is needed later than 
**     upon initialization
**	20-Jul-2000 (hanch04)
**	    Call ping_iigcn rather than greping the process list for UNIX.
**	    This should be a generic change.
**  20-Dec-2000 (noifr01)
**     (SIR 103548) now call ping_iigcn rather than searching for a process.
**     This is required for supporting multiple installations.
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  09-May-2001 (schph01)
**     SIR 102509  on OT_LOCATION type, recover the new RAWPCT parameter.
**  06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
**  09-Jul-2001 (hanje04)
*	Reformatted line breaks in some lines becuase of compilation problems
**	on Solaris.
**  17-Jul-2001 (noifr01)
**  removed a "comment like" line with 2 * signs, that was apparently there by
**  mistake and didn't compile
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  02-Sep-2002 (schph01)
**    bug 108645 Replace the call of CGhostname() by the GetLocalVnodeName()
**    function.( XX.XXXXX.gcn.local_vnode parameter in config.dat)
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  10-Nov-2004 (schph01)
**    (bug 113426) manage OT_ICE_MAIN no details needed for this type.
******************************************************************************/

#include "compat.h"
#include "gc.h"

#include "dba.h"
#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"
#include "domdisp.h"
#include "msghandl.h"
#include "dll.h"
#include "main.h"
#include "winutils.h"
#include "getdata.h"
#include "monitor.h"
#include "prodname.h"
#include "resource.h"
#include "extccall.h"

static BOOL bIsNodesListInSpecialState = FALSE;
extern BOOL isNodesListInSpecialState()
{
	 return bIsNodesListInSpecialState;
}

static int DOMQuickRefreshList(pvoid, iobjecttype, strings,
                               nbobjects,bWithChildren) 
void *pvoid       ;
int  iobjecttype  ;
LPUCHAR *strings  ;
int nbobjects     ;
BOOL bWithChildren;
{
   int QuickInfoType;
   if (bWithChildren)
      QuickInfoType=INFOFROMPARENTIMM;
   else 
      QuickInfoType=INFOFROMPARENT;

   switch (iobjecttype) {
      case OT_INDEX:
         {
            struct TableData * ptabledata= (struct TableData *)pvoid;
            if (nbobjects)
               break; /* no quick refresh because owner needs extra request*/
            if (!ptabledata->lpIndexData) {
              ptabledata->lpIndexData = (struct IndexData * )
                           ESL_AllocMem(sizeof(struct IndexData));
              if (!ptabledata->lpIndexData) 
                 return RES_ERR;
            }
            ptabledata->nbIndexes  = nbobjects;
            ptabledata->iIndexDataQuickInfo  = QuickInfoType;
            UpdRefreshParams(&(ptabledata->IndexesRefreshParms),OT_INDEX);
            ptabledata->indexeserrcode=RES_SUCCESS;
         }
         break;
      case OT_INTEGRITY:
         {
            struct TableData * ptabledata= (struct TableData *)pvoid;
            if (nbobjects)
               break; /* no quick refresh because owner needs extra request*/
            if (!ptabledata->lpIntegrityData) {
              ptabledata->lpIntegrityData = (struct IntegrityData * )
                           ESL_AllocMem(sizeof(struct IntegrityData));
              if (!ptabledata->lpIntegrityData) 
                 return RES_ERR;
            }
            ptabledata->nbintegrities  = nbobjects;
            ptabledata->iIntegrityDataQuickInfo  = QuickInfoType;
            UpdRefreshParams(&(ptabledata->IntegritiesRefreshParms),OT_INTEGRITY);
            ptabledata->integritieserrcode=RES_SUCCESS;
         }
         break;
      case OT_TABLELOCATION:
         {
            struct TableData * ptabledata= (struct TableData *)pvoid;
            if (nbobjects!=1)
               break; /* to be changed with some of the following code if */
                      /*   system files are changed needs extra request   */
            if (!ptabledata->lpTableLocationData) {
              ptabledata->lpTableLocationData = (struct LocationData * )
                           ESL_AllocMem(sizeof(struct LocationData));
              if (!ptabledata->lpTableLocationData) 
                 return RES_ERR;
            }
            fstrncpy(ptabledata->lpTableLocationData->LocationName,strings[0],
                     MAXOBJECTNAME);

            /* the area of the location is not searched here in */
            /* because the refresh of locations may not be      */
            /* synchronized. It is done in domdgetd.c           */

            ptabledata->nbTableLocations  = nbobjects;
            ptabledata->iTableLocDataQuickInfo  = QuickInfoType;
            UpdRefreshParams(&(ptabledata->TableLocationsRefreshParms),
                             OT_TABLELOCATION);
            ptabledata->tablelocationserrcode=RES_SUCCESS;
         }
         break;
      default:
         return myerror(ERR_OBJECTNOEXISTS);
         break;
   }
   return RES_SUCCESS;
}

long PrepareGlobalDOMUpd(BOOL bReset) { /* to be called before multiple */
   if (bReset)                          /* possible updates of the same */
      DOMUpdver=0;                      /* branches in the cache. If 0L */
   else                                 /* is returned, UpdateDOMData   */
      DOMUpdver++;                      /* should be called with bUnique*/
   return DOMUpdver;                    /* to false and OT_VIRTNODE for */
}                                       /* all nodes with display       */

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


int UpdateDOMDataDetail(hnodestruct,iobjecttype,level,parentstrings,
                        bWithSystem,bWithChildren,bOnlyIfExist,bUnique,
                        bWithDisplay)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
BOOL    bWithChildren;
BOOL    bOnlyIfExist;
BOOL    bUnique;
BOOL    bWithDisplay;
{
         
   int   i, iold, iobj, iobj2, iret, iret1, icmp;
   struct  ServConnectData * pdata1;
   UCHAR   buf[MAXOBJECTNAME];  
   UCHAR   buffilter[MAXOBJECTNAME];
   UCHAR   extradata[EXTRADATASIZE];
   LPUCHAR aparents[MAXPLEVEL];
   UCHAR   bufparent1WithOwner[MAXOBJECTNAME];
   void *  ptemp;
   int     iReplicVersion;

   if (hnodestruct<0 || hnodestruct >= imaxnodes) 
      return myerror(ERR_UPDATEDOMDATA);

   if (HasLoopInterruptBeenRequested())
		return RES_ERR;

	if (iobjecttype == OT_SCHEMAUSER_TABLE)
		iobjecttype = OT_TABLE, level = 1;
	if (iobjecttype == OT_SCHEMAUSER_VIEW)
		iobjecttype = OT_VIEW, level = 1;
	if (iobjecttype == OT_SCHEMAUSER_PROCEDURE)
		iobjecttype = OT_PROCEDURE, level = 1;


   pdata1 = virtnode[hnodestruct].pdata;

   if (!pdata1 && iobjecttype!=OT_NODE
			   && iobjecttype!=OT_NODE_OPENWIN
               && iobjecttype!=OT_NODE_SERVERCLASS
               && iobjecttype!=OT_NODE_LOGINPSW
			   && iobjecttype!=OT_NODE_CONNECTIONDATA
			   && iobjecttype!=OT_USER
			   && iobjecttype!=OT_NODE_ATTRIBUTE) 
      return myerror(ERR_UPDATEDOMDATA);


   iret = RES_SUCCESS;

   switch (iobjecttype) {
      case OT_VIRTNODE  :
         if (!bWithChildren)
            break;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_PROFILE ,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_USER    ,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_GROUP   ,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ROLE    ,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_LOCATION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OTLL_DBSECALARM,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_ROLE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_DBUSER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_DBCONNECTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_WEBUSER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_PROFILE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_SERVER_APPLICATION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_SERVER_LOCATION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_SERVER_VARIABLE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         iret1 = UpdateDOMDataDetail(hnodestruct,OTLL_DBGRANTEE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         if (iret1 != RES_SUCCESS)
            iret = iret1;
         break;
      case OT_MON_ALL  :
         bWithChildren=TRUE;
         bOnlyIfExist=TRUE;
            // OT_MON_LOCKLIST needs to be first because it's sub-branches (LOCKS) are used for Server's subbranches
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_LOCKLIST,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_SERVER, 0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OTLL_MON_RESOURCE, 0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_LOGPROCESS,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_LOGDATABASE,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_TRANSACTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
            iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_REPLIC_SERVER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
            if (iret1 != RES_SUCCESS)
               iret = iret1;
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pServerdatanew->ServerDta),
                                       NULL,extradata);

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
                  icmp = CompareMonInfo(OT_MON_SERVER,
                                        &pServerdatanew->ServerDta,
                                        &pServerdataold->ServerDta);
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
               iret1 = DBAGetNextObject((LPUCHAR)&(pServerdatanew->ServerDta),buffilter,extradata);
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
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
Serverchildren:
            UpdRefreshParams(&(pdata1->ServerRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all Servers */
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_SESSION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_CONNECTED_USER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_ACTIVE_USER,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_TRANSACTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_CURSOR,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_FILEINFO,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_ICE_DB_CONNECTION,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLockListdatanew->LockListDta),
                                       NULL,extradata);

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
                  icmp = CompareMonInfo(OT_MON_LOCKLIST,
                                        &pLockListdatanew->LockListDta,
                                        &pLockListdataold->LockListDta);
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
               iret1 = DBAGetNextObject((LPUCHAR)&(pLockListdatanew->LockListDta),buffilter,extradata);
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
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
LockListchildren:
            UpdRefreshParams(&(pdata1->LockListRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all LockLists */
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_LOCKLIST_LOCK,0,NULL,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pResourcedatanew->ResourceDta),
                                       NULL,extradata);

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
                  icmp = CompareMonInfo(OTLL_MON_RESOURCE,
                                       &pResourcedatanew->ResourceDta,
                                       &pResourcedataold->ResourceDta);
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
               iret1 = DBAGetNextObject((LPUCHAR)&(pResourcedatanew->ResourceDta),buffilter,extradata);
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
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
Resourcechildren:
            UpdRefreshParams(&(pdata1->ResourceRefreshParms),OT_MON_ALL);
            if (bWithChildren) {
//              aparents[0]=NULL; /* for all Resources */
//              iret1 = UpdateDOMDataDetail(hnodestruct,OT_MON_RES_LOCK,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogProcessdatanew->LogProcessDta),
                                       NULL,extradata);

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
               iret1 = DBAGetNextObject((LPUCHAR)&(pLogProcessdatanew->LogProcessDta),buffilter,extradata);
            }
            pdata1->nbLogProcesses = i;
            freeLogProcessData(pdata1->lpLogProcessData);
            pdata1->lpLogProcessData = pLogProcessdatanew0;
            pdata1->logprocesseserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogDBdatanew->LogDBDta),
                                       NULL,extradata);

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
               iret1 = DBAGetNextObject((LPUCHAR)&(pLogDBdatanew->LogDBDta),buffilter,extradata);
            }
            pdata1->nbLogDB = i;
            freeLogDBData(pdata1->lpLogDBData);
            pdata1->lpLogDBData = pLogDBdatanew0;
            pdata1->logDBerrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       (LPUCHAR)&(pLogTransactdatanew->LogTransactDta),
                                       NULL,extradata);

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
               iret1 = DBAGetNextObject((LPUCHAR)&(pLogTransactdatanew->LogTransactDta),buffilter,extradata);
            }
            pdata1->nbLogTransact = i;
            freeLogTransactData(pdata1->lpLogTransactData);
            pdata1->lpLogTransactData = pLogTransactdatanew0;
            pdata1->LogTransacterrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pSessiondatanew->SessionDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pSessiondatanew->SessionDta),NULL,extradata);
               }
               pServerdata->nbServerSessions = i;
               freeSessionData(pServerdata->lpServerSessionData);
               pServerdata->lpServerSessionData = pSessiondatanew0;
               pServerdata->ServerSessionserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceConnuserdatanew->IceConnUserDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIceConnuserdatanew->IceConnUserDta),NULL,extradata);
               }
               pServerdata->nbiceconnusers = i;
               freeIceConnectUsersData(pServerdata->lpIceConnUsersData);
               pServerdata->lpIceConnUsersData = pIceConnuserdatanew0;
               pServerdata->iceConnuserserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceActiveuserdatanew->IceActiveUserDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIceActiveuserdatanew->IceActiveUserDta),NULL,extradata);
               }
               pServerdata->nbiceactiveusers = i;
               freeIceActiveUsersData(pServerdata->lpIceActiveUsersData);
               pServerdata->lpIceActiveUsersData = pIceActiveuserdatanew0;
               pServerdata->iceactiveuserserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIcetransactdatanew->IceTransactDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIcetransactdatanew->IceTransactDta),NULL,extradata);
               }
               pServerdata->nbicetransactions = i;
               freeIcetransactionsData(pServerdata->lpIceTransactionsData);
               pServerdata->lpIceTransactionsData = pIcetransactdatanew0;
               pServerdata->icetransactionserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceCursordatanew->IceCursorDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIceCursordatanew->IceCursorDta),NULL,extradata);
               }
               pServerdata->nbicecursors = i;
               freeIceCursorsData(pServerdata->lpIceCursorsData);
               pServerdata->lpIceCursorsData = pIceCursordatanew0;
               pServerdata->icecursorsserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceCachedatanew->IceCacheDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIceCachedatanew->IceCacheDta),NULL,extradata);
               }
               pServerdata->nbicecacheinfo = i;
               freeIceCacheData(pServerdata->lpIceCacheInfoData);
               pServerdata->lpIceCacheInfoData = pIceCachedatanew0;
               pServerdata->icecacheinfoerrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_SERVER, parentstrings,&(pServerdata->ServerDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *) &pServerdata->ServerDta,
                                       TRUE,
                                       (LPUCHAR)&(pIceDbConnectdatanew->IceDbConnectDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pIceDbConnectdatanew->IceDbConnectDta),NULL,extradata);
               }
               pServerdata->nbicedbconnectinfo = i;
               freeIceDbConnectData(pServerdata->lpIceDbConnectData);
               pServerdata->lpIceDbConnectData = pIceDbConnectdatanew0;
               pServerdata->icedbconnectinfoerrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
                  if (CompareMonInfo(OT_MON_LOCKLIST, parentstrings,&(pLockListdata->LockListDta))) 
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       (LPUCHAR *)&pLockListdata->LockListDta,
                                       TRUE,
                                       (LPUCHAR)&(pLockdatanew->LockDta),
                                       NULL,extradata);
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
                  iret1 = DBAGetNextObject((LPUCHAR)&(pLockdatanew->LockDta),NULL,extradata);
               }
               pLockListdata->nbLocks = i;
               freeLockData(pLockListdata->lpLockData);
               pLockListdata->lpLockData = pLockdatanew0;
               pLockListdata->lockserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
//               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
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
//                  iret1 = DBAGetNextObject((LPUCHAR)&(pLockdatanew->LockDta),NULL,extradata);
//               }
//               pResourcedata->nbResourceLocks = i;
//               freeLockData(pResourcedata->lpResourceLockData);
//               pResourcedata->lpResourceLockData = pLockdatanew0;
//               pResourcedata->ResourceLockserrcode=RES_SUCCESS;
//               if (bWithDisplay) {
//                  DOMDisableDisplay(hnodestruct, 0);
//                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
//                                        FALSE,ACT_BKTASK,0L,0);
//                  DOMEnableDisplay(hnodestruct, 0);
//               }
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

      case OT_NODE  :
         {
           struct MonNodeData  * pMonNodeDatanew,*pMonNodeDatanew0,
                               * pMonNodeDataold;
           LPMONNODEDATA    p1;
		   char LocalHostName[MAXOBJECTNAME];
		   BOOL bLocalHNFound = FALSE;
		   BOOL bExistLHNNode = FALSE;

			BOOL bNodeFromCmdArgs = FALSE;
           GetLocalVnodeName (LocalHostName,MAXOBJECTNAME);
           if (LocalHostName[0] != '\0')
				bLocalHNFound = TRUE;
        
           pMonNodeDataold  = lpMonNodeData;
           if (bOnlyIfExist && !lpMonNodeData)
              break;

           if (bUnique && MonNodesRefreshParms.LastDOMUpdver==DOMUpdver) 
              goto MonNodeEnd;

           pMonNodeDatanew0 = (struct MonNodeData *)
                              ESL_AllocMem(sizeof(struct MonNodeData));
           if (!pMonNodeDatanew0) 
              return RES_ERR;

           pMonNodeDatanew  = pMonNodeDatanew0;
           if (!ping_iigcn()) {
              int ret;
			  char bufmsg[400];
              PushVDBADlg();
			  wsprintf(bufmsg, ResourceString((UINT)IDS_MSG_INSTALL_NOT_STARTED), VDBA_GetInstallName4Messages());
              ret = TS_MessageBox(TS_GetFocus(), bufmsg, NULL, MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
              PopVDBADlg();
              if (ret == IDYES) {
                long lval;
                ShowHourGlass();    // since application will look like inactive
                lval = LaunchIngstartDialogBox(NULL);
                RemoveHourGlass();
                if (lval) {
                  iret=RES_ERR;
                  ESL_FreeMem(pMonNodeDatanew0);
                  break;
                }
              }
              else {
                iret=RES_ERR;
                ESL_FreeMem(pMonNodeDatanew0);
                break;
              }
           }
           ShowHourGlass();
           iret1=NodeLLInit();
			{
				char bufnode[100];
				BOOL bGottenFromCommandArgs = GetContextDrivenNodeName(bufnode);
				bIsNodesListInSpecialState = FALSE;
				if (bGottenFromCommandArgs) { /* specified (..) to be FALSE later than initialization */
					NODEDATAMIN MonNodeDta;
					freeMonNodeData(lpMonNodeData);
					lpMonNodeData = NULL;
					if (!x_stricmp(bufnode,ResourceString((UINT)IDS_I_LOCALNODE)))
						MonNodeDta.bIsLocal = TRUE;
					else
						MonNodeDta.bIsLocal = FALSE;
					x_strcpy(MonNodeDta.NodeName,bufnode);
					MonNodeDta.isSimplifiedModeOK = FALSE;
					MonNodeDta.bWrongLocalName = FALSE;
					MonNodeDta.bInstallPassword = FALSE;
					MonNodeDta.bTooMuchInfoInInstallPassword = FALSE;
					MonNodeDta.inbConnectData = 0;
					memset(&(MonNodeDta.ConnectDta),'\0',sizeof(MonNodeDta.ConnectDta));
					MonNodeDta.inbLoginData = 0;
					memset(&(MonNodeDta.LoginDta),'\0',sizeof(MonNodeDta.LoginDta));
					pMonNodeDatanew->MonNodeDta = MonNodeDta;
					pMonNodeDatanew=GetNextMonNode(pMonNodeDatanew);
					bNodeFromCmdArgs = TRUE;
					i=1;
					bIsNodesListInSpecialState = TRUE;
					goto cont;
				}
			}
           if (iret1==RES_SUCCESS) 
              iret1=NodeLLFillNodeLists();

           if (iret1!=RES_SUCCESS)  {
              iret1=RES_ERR;
              if (!( lpMonNodeData &&
                    AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                 nbMonNodes=0;
                 monnodeserrcode=iret1;
                 iret = iret1;
              }
              goto MonNodeEnd;
              break;
           }
           else {
              p1= GetFirstLLNode();
              freeMonNodeData(lpMonNodeData);
              lpMonNodeData = NULL;
              for (i=0,iold=0;; i++, pMonNodeDatanew=GetNextMonNode(pMonNodeDatanew)) 
{
                if (!pMonNodeDatanew)
                   iret1 = RES_ERR;
                if (p1) {
					if (p1->MonNodeDta.NodeName[0]=='\0') {
                      p1=p1->pnext;
					  if (!p1)
						 break;
					}
				  
					if (p1->MonNodeDta.bWrongLocalName)
						bExistLHNNode = TRUE;
                }
                if (p1) {
                  pMonNodeDatanew->MonNodeDta = p1->MonNodeDta;
                  p1=p1->pnext;
                }
                else
                  break;
              }

			if (bExistLHNNode) {
				char buf1[800];
				wsprintf(buf1,ResourceString(IDS_MSG_NODE_EQUAL_LOCALVNODE),LocalHostName,LocalHostName);
				PushVDBADlg();
				TS_MessageBox(TS_GetFocus(),buf1,NULL,MB_ICONHAND | MB_OK | MB_TASKMODAL);
				PopVDBADlg();
			}
cont:
              nbMonNodes = i;
              lpMonNodeData = pMonNodeDatanew0;
              monnodeserrcode=RES_SUCCESS;
              if (bWithDisplay) {
                 DOMDisableDisplay(hnodestruct, 0);
                 DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                       FALSE,ACT_BKTASK,0L,0);
                 DOMEnableDisplay(hnodestruct, 0);
              }
           }
           UpdRefreshParams(&(MonNodesRefreshParms),OT_NODE);
			if (bNodeFromCmdArgs)
				goto MonNodeEnd;
           iret1 = UpdateDOMDataDetail(hnodestruct,OT_NODE_OPENWIN,0,NULL,
                                 bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
           if (iret1 != RES_SUCCESS)
               iret = iret1;
           iret1 = UpdateDOMDataDetail(hnodestruct,OT_NODE_SERVERCLASS,0,NULL,
                                 bWithSystem,bWithChildren,TRUE,bUnique,bWithDisplay);
           if (iret1 != RES_SUCCESS)
               iret = iret1;

           // Emb: Need to update users list for issue 92465
           iret1 = UpdateDOMDataDetail(hnodestruct,OT_USER,0,NULL,
                                 bWithSystem,bWithChildren,TRUE,bUnique,bWithDisplay);
           if (iret1 != RES_SUCCESS)
               iret = iret1;

           // update connect/login data subbranches 

           for (p1=lpMonNodeData,i=0;i<nbMonNodes;i++,p1=GetNextMonNode(p1)) {
             LPNODELOGINDATA     plogdata      , plogdatall ;
             LPNODECONNECTDATA   pconnectdata  , pconnectdatall ;
			 LPNODEATTRIBUTEDATA pattributedata, pattributedataall;

             int i1;
             p1->lpNodeConnectionData=ESL_AllocMem(sizeof(NODECONNECTDATA));
             p1->lpNodeAttributeData =ESL_AllocMem(sizeof(NODEATTRIBUTEDATA));
             p1->lpNodeLoginData     =ESL_AllocMem(sizeof(NODELOGINDATA));
             if (!p1->lpNodeConnectionData ||!p1->lpNodeLoginData){
                iret= RES_ERR;
                break;
             }
             pconnectdata   = p1->lpNodeConnectionData;
			 pattributedata = p1->lpNodeAttributeData;
             plogdata       = p1->lpNodeLoginData;

             i1=0;  // get connections
             for (pconnectdatall=GetFirstLLConnectData(); pconnectdatall;
                  pconnectdatall=pconnectdatall->pnext) {
                if (!x_strcmp(pconnectdatall->NodeConnectionDta.NodeName,
                      p1->MonNodeDta.NodeName)) {
                     *pconnectdata = *pconnectdatall;
                     pconnectdata->pnext=NULL;
                     pconnectdata=GetNextNodeConnection(pconnectdata);
                     i1++;
                }
             }
             p1->nbNodeConnections=i1;
             p1->NodeConnectionerrcode=RES_SUCCESS;
             p1->HasSystemNodeConnections= TRUE;

             i1=0;  // get attributes
             for (pattributedataall=GetFirstLLAttributeData(); pattributedataall;
                  pattributedataall=pattributedataall->pnext) {
                if (!x_strcmp(pattributedataall->NodeAttributeDta.NodeName,
                      p1->MonNodeDta.NodeName)) {
                     *pattributedata = *pattributedataall;
                     pattributedata->pnext=NULL;
                     pattributedata=GetNextNodeAttribute(pattributedata);
                     i1++;
                }
             }
             p1->nbNodeAttributes=i1;
             p1->NodeAttributeerrcode=RES_SUCCESS;
             p1->HasSystemNodeAttributes= TRUE;

             i1=0;   // get logins
             for (plogdatall=GetFirstLLLoginData(); plogdatall;
                  plogdatall=plogdatall->pnext) {
                if (!x_strcmp(plogdatall->NodeLoginDta.NodeName,
                      p1->MonNodeDta.NodeName)) {
                     *plogdata = *plogdatall;
                     plogdata->pnext=NULL;
                     plogdata=GetNextNodeLogin(plogdata);
                     i1++;
                }
             }
             p1->nblogins=i1;
             p1->loginerrcode = RES_SUCCESS;
             p1->HasSystemLogins= TRUE;
           }
           // end of update of connect/login data sub-branches
MonNodeEnd:
           UpdRefreshParams(&MonNodesRefreshParms,OT_NODE);
           NodeLLTerminate();
           RemoveHourGlass();
         }
         break;

      case OT_NODE_OPENWIN :
         {
            struct MonNodeData     * pmonnodedata;
            struct WindowData * pwindowdatanew, * pwindowdatanew0;
            struct WindowData * ls0, * ls1, *ls2;
			   BOOL bDone;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
           // if (level!=1)
           //    return myerror(ERR_LEVELNUMBER);
            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<nbMonNodes;
                        iobj++, pmonnodedata=GetNextMonNode(pmonnodedata)) {
               HWND        hwndCurDoc;       // for loop on documents
               if (parentstrings) {
                  if (CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pmonnodedata->lpWindowData)
                  continue;

               if (bUnique && pmonnodedata->WindowRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto windowchildren;

               pwindowdatanew0 = (struct WindowData * )
                                  ESL_AllocMem(sizeof(struct WindowData));
               if (!pwindowdatanew0) 
                  return RES_ERR;

               pwindowdatanew = pwindowdatanew0;

               hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
               
               while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
               {
                   hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
               }
               i=0;
               while (hwndCurDoc)
               {
                  UCHAR NodeName[200];
                  BOOL bHasGW, bHasUser;
                  UCHAR szServerClass[100];
                  UCHAR szUser[MAXOBJECTNAME];

                  // Special management to exclude NODES in window mode
                  char * lpCacheNodeName = "";
                  int docType = QueryDocType(hwndCurDoc);
                  if (docType != TYPEDOC_NODESEL /* && docType != TYPEDOC_UNKNOWN */ )
                    lpCacheNodeName = GetVirtNodeName(GetMDINodeHandle (hwndCurDoc));
                  else
                    lpCacheNodeName = "";

                  if (!lpCacheNodeName ) {
                     myerror(ERR_REFRESH_WIN);
                     lpCacheNodeName="";
                  }
                  x_strcpy(NodeName,lpCacheNodeName);
                  bHasGW = GetGWClassNameFromString(NodeName,szServerClass);
                  RemoveGWNameFromString(NodeName);

                  bHasUser = GetConnectUserFromString(NodeName,szUser);
                  RemoveConnectUserFromString(NodeName);


                  if (!x_strcmp(NodeName,pmonnodedata->MonNodeDta.NodeName)) {
                      char buf[200];
                      char *pcloc = buf;
                      if (i)
                        pwindowdatanew=GetNextNodeWindow(pwindowdatanew);
                      GetWindowText (hwndCurDoc, buf, sizeof(buf));

                      if (!x_strncmp(buf,pmonnodedata->MonNodeDta.NodeName,x_strlen(pmonnodedata->MonNodeDta.NodeName))) {
                        pcloc+=x_strlen(pmonnodedata->MonNodeDta.NodeName);
                        if (!x_strncmp(pcloc," - ",2))
                          pcloc+=3;
                      }

                      fstrncpy(pwindowdatanew->WindowDta.Title      ,pcloc        ,sizeof(pwindowdatanew->WindowDta.Title));
                      fstrncpy(pwindowdatanew->WindowDta.ServerClass,szServerClass,sizeof(pwindowdatanew->WindowDta.ServerClass));
                      fstrncpy(pwindowdatanew->WindowDta.ConnectUserName,szUser   ,sizeof(pwindowdatanew->WindowDta.ConnectUserName));
                      
                      pwindowdatanew->WindowDta.hwnd = hwndCurDoc;
                      i++;
                  }

                  //
                  // Get next document handle (with loop to skip the icon title windows)
                  //

                  hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
                  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
                  {
                      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
                  }
               }

               pmonnodedata->nbwindows = i;
               freeWindowData(pmonnodedata->lpWindowData);

               // sort the linked list 
               bDone=FALSE;
               while (!bDone) {
                 ls0 = pwindowdatanew0;
                 ls1=ls0->pnext;
                 if (!ls1)
                   break;
                 ls2=ls1->pnext;
                 if (x_stricmp(ls0->WindowDta.Title,ls1->WindowDta.Title)>0) {
                   pwindowdatanew0=ls1;
                   pwindowdatanew0->pnext=ls0;
                   ls0->pnext=ls2;
                   continue;
                 }
                 if (!ls2)
                    break;
                 while (!bDone) {
                   if (!ls2) {
                      bDone=TRUE;
                      break;
                   }
                   if (x_stricmp(ls1->WindowDta.Title,ls2->WindowDta.Title)>0) {
                     ls0->pnext=ls2;
                     ls1->pnext=ls2->pnext;
                     ls2->pnext=ls1;
                     break;
                   }
                   ls0=ls1;
                   ls1=ls0->pnext;
                   if (!ls1) {
                      bDone=TRUE;
                      break;
                   }
                   ls2=ls1->pnext;

                 }
               }
               // end of sort/
               pmonnodedata->lpWindowData = pwindowdatanew0;
               pmonnodedata->windowserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
windowchildren:
               UpdRefreshParams(&(pmonnodedata->WindowRefreshParms),
                                OT_NODE);
               /* no children */
               iret=iret; /* otherwise compile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;
      case OT_NODE_SERVERCLASS :
         {
            struct MonNodeData     * pmonnodedata;
            struct NodeServerClassData * pserverclassdatanew, * pserverclassdatanew0;
            BOOL bParentFound = parentstrings? FALSE: TRUE;
            LPUCHAR * lpgwlist;
            
            pmonnodedata = lpMonNodeData;
            if (!pmonnodedata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<nbMonNodes;
                        iobj++, pmonnodedata=GetNextMonNode(pmonnodedata)) {
               if (parentstrings) {
                  if (CompareMonInfo(OT_NODE, parentstrings,&(pmonnodedata->MonNodeDta))) 
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pmonnodedata->lpServerClassData)
                  continue;

               if (bUnique && pmonnodedata->ServerClassRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto serverclasschildren;

               pserverclassdatanew0 = (struct NodeServerClassData * )
                                  ESL_AllocMem(sizeof(struct NodeServerClassData));
               if (!pserverclassdatanew0) 
                  return RES_ERR;

               pserverclassdatanew = pserverclassdatanew0;
               iret=RES_SUCCESS;

               for (i=0,lpgwlist = GetGWlist(pmonnodedata->MonNodeDta.NodeName);; i++, lpgwlist++, pserverclassdatanew=GetNextServerClass(pserverclassdatanew)) 
{
                   if (!lpgwlist) { // error management, (at loop init only)
                        if(!(pmonnodedata->lpServerClassData &&
                             AskIfKeepOldState(iobjecttype,RES_ERR,parentstrings))){
                           pmonnodedata->nbserverclasses=0;
                           pmonnodedata->serverclasseserrcode=RES_ERR;
                           iret=RES_ERR;
                        }
                        if (!pmonnodedata->lpServerClassData)
                           pmonnodedata->lpServerClassData = pserverclassdatanew0;
                        else 
                           freeNodeServerClassData(pserverclassdatanew0);
                        goto serverclasschildren;
                        break;
                   }
                   if (! (*lpgwlist))
                     break;
                   fstrncpy(pserverclassdatanew->NodeSvrClassDta.NodeName,
                            pmonnodedata->MonNodeDta.NodeName,
                            sizeof(pserverclassdatanew->NodeSvrClassDta.NodeName));
                   pserverclassdatanew->NodeSvrClassDta.bIsLocal           = pmonnodedata->MonNodeDta.bIsLocal;
                   pserverclassdatanew->NodeSvrClassDta.isSimplifiedModeOK = pmonnodedata->MonNodeDta.isSimplifiedModeOK;
                   fstrncpy(pserverclassdatanew->NodeSvrClassDta.ServerClass,*lpgwlist,
                          sizeof(pserverclassdatanew->NodeSvrClassDta.ServerClass));
               }

               pmonnodedata->nbserverclasses = i;
               freeNodeServerClassData(pmonnodedata->lpServerClassData);
               pmonnodedata->lpServerClassData = pserverclassdatanew0;
               pmonnodedata->serverclasseserrcode=RES_SUCCESS;
               { 
				   int hnode;
                   hnode=OpenNodeStruct(pmonnodedata->MonNodeDta.NodeName);
				   if (hnode>=0)
						CloseNodeStruct(hnode,TRUE);
			   }

               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
serverclasschildren:
               UpdRefreshParams(&(pmonnodedata->ServerClassRefreshParms),
                                OT_NODE);
               /* no children */
               iret=iret; /* otherwise compile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;

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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);

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
               iret1 =DBAGetNextObject(buf,buffilter,extradata);
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
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
DBchildren:
            UpdRefreshParams(&(pdata1->DBRefreshParms),OT_DATABASE);
            if (bWithChildren) {
               int itype[]={OT_TABLE,
                            OT_VIEW,
                            OT_PROCEDURE,
                            OT_SEQUENCE,
                            OT_SCHEMAUSER,
                            OT_SYNONYM,
                            OT_DBEVENT,
                            OT_REPLIC_CONNECTION,
                            OT_REPLIC_REGTABLE,
                            OT_REPLIC_MAILUSER,
                            OTLL_REPLIC_CDDS,
                            OTLL_GRANTEE,
                            OTLL_SECURITYALARM };
               aparents[0]=NULL; /* for all databases */
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  iret1 = UpdateDOMDataDetail(hnodestruct,itype[i],1,aparents,
                                        bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
               }
            }
         }
         break;


      case OT_ICE_WEBUSER              :
      case OT_ICE_PROFILE              :
         {
            struct IceObjectWithRolesAndDBData *pobjdatanew, *pobjdatanew0, *pobjdataold;
            struct IceObjectWithRolesAndDBData *ptempold;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;
			switch (iobjecttype) {
				case OT_ICE_WEBUSER:
					ppstart       = &(pdata1 ->lpIceWebUsersData);
					prefreshparms = &(pdata1->IceWebUsersRefreshParms);
					pnresult      = &(pdata1->nbicewebusers);
					piresultcode  = &(pdata1->icewebusererrcode);
					break;
				case OT_ICE_PROFILE:
					ppstart       = &(pdata1 ->lpIceProfilesData);
					prefreshparms = &(pdata1->IceProfilesRefreshParms);
					pnresult      = &(pdata1->nbiceprofiles);
					piresultcode  = &(pdata1->iceprofileserrcode);
					break;
			}

            if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
               goto IceChildren00;

            if (bOnlyIfExist && !*ppstart)
               break;

            pobjdataold  = *ppstart;
            pobjdatanew0 = (struct IceObjectWithRolesAndDBData * )
                                      ESL_AllocMem(sizeof(struct IceObjectWithRolesAndDBData));
            if (!pobjdatanew0)
               return RES_ERR;

            pobjdatanew = pobjdatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++,pobjdatanew=GetNextObjWithRoleAndDb(pobjdatanew)) {
               /* pgroupdatanew points to zeroed area as the list is new */
               if (!pobjdatanew)
                  iret1 = RES_ERR;
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(*ppstart  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        *pnresult=0;
                        *piresultcode=iret1;
                        iret=iret1;
                     }
                     if (!*ppstart)
                        *ppstart = pobjdatanew0;
                     else 
                        freeIceObjectWithRolesAndDBData(pobjdatanew0);
                     goto IceChildren00;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pobjdataold && iold<*pnresult) {
                  icmp = x_strcmp(buf,pobjdataold->ObjectName);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pobjdatanew->ptemp = pobjdataold;
                     pobjdataold=GetNextObjWithRoleAndDb(pobjdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pobjdataold=GetNextObjWithRoleAndDb(pobjdataold);
               }
               fstrncpy(pobjdatanew->ObjectName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            *pnresult = i;
            for (i=0,pobjdatanew=pobjdatanew0; i<*pnresult;
                            i++,pobjdatanew=GetNextObjWithRoleAndDb(pobjdatanew)) {
               if (pobjdatanew->ptemp) {
                  ptemp    = (void *) pobjdatanew->pnext;
                  ptempold =          pobjdatanew->ptemp;
                  memcpy(pobjdatanew,ptempold,sizeof (struct IceObjectWithRolesAndDBData));
                  pobjdatanew->pnext = (struct IceObjectWithRolesAndDBData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct IceObjectWithRolesAndDBData));
                  ptempold->pnext = (struct IceObjectWithRolesAndDBData *) ptemp;
               }
            }
            freeIceObjectWithRolesAndDBData(*ppstart);
            *ppstart = pobjdatanew0;
            *piresultcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
IceChildren00:
               UpdRefreshParams(prefreshparms,OT_ICE_GENERIC);
            if (bWithChildren) {
				int itype1,itype2;
				aparents[0]=NULL; /* for all groups */
				switch (iobjecttype) {
					case OT_ICE_WEBUSER:
						itype1=OT_ICE_WEBUSER_ROLE;
						itype2=OT_ICE_WEBUSER_CONNECTION;
						break;
					case OT_ICE_PROFILE:
						itype1=OT_ICE_PROFILE_ROLE;
						itype2=OT_ICE_PROFILE_CONNECTION;
						break;
				}
				iret1 = UpdateDOMDataDetail(hnodestruct,itype1,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
				iret1 = UpdateDOMDataDetail(hnodestruct,itype2,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
            }
         }
         break;

      case OT_ICE_WEBUSER_ROLE         :
      case OT_ICE_WEBUSER_CONNECTION   :
      case OT_ICE_PROFILE_ROLE         :
      case OT_ICE_PROFILE_CONNECTION   :
         {
            struct IceObjectWithRolesAndDBData     * pparentdata;
            struct SimpleLeafObjectData * pchilddatanew, * pchilddatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;
			int nbparents;

            if (level!=1)
               return myerror(ERR_LEVELNUMBER);

			switch (iobjecttype) {
				case OT_ICE_WEBUSER_ROLE         :
				case OT_ICE_WEBUSER_CONNECTION   :
					pparentdata= pdata1 -> lpIceWebUsersData;
					nbparents  = pdata1 -> nbicewebusers;
					break;
				case OT_ICE_PROFILE_ROLE         :
				case OT_ICE_PROFILE_CONNECTION   :
					pparentdata= pdata1 -> lpIceProfilesData;
					nbparents  = pdata1 -> nbiceprofiles;
					break;
			}
            if (!pparentdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<nbparents;
                        iobj++, pparentdata=GetNextObjWithRoleAndDb(pparentdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pparentdata->ObjectName))
                     continue;
               }
               bParentFound = TRUE;
				switch (iobjecttype) {
					case OT_ICE_WEBUSER_ROLE:
					case OT_ICE_PROFILE_ROLE:
						ppstart       = &(pparentdata->lpObjectRolesData);
						prefreshparms = &(pparentdata->ObjectRolesRefreshParms);
						pnresult      = &(pparentdata->nbobjectroles);
						piresultcode  = &(pparentdata->objectroleserrcode);
						break;
					case OT_ICE_WEBUSER_CONNECTION:
					case OT_ICE_PROFILE_CONNECTION:
						ppstart       = &(pparentdata->lpObjectDbData);
						prefreshparms = &(pparentdata->ObjectDbRefreshParms);
						pnresult      = &(pparentdata->nbobjectdbs);
						piresultcode  = &(pparentdata->objectdbserrcode);
						break;
				}

               if (bOnlyIfExist && !*ppstart)
                  continue;

               if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
                  goto IceRoleConnectChildchildren;

               pchilddatanew0 = (struct SimpleLeafObjectData * )
                                  ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
               if (!pchilddatanew0) 
                  return RES_ERR;

               pchilddatanew = pchilddatanew0;
               aparents[0]=pparentdata->ObjectName; 
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       NULL,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pchilddatanew=
                                   GetNextSimpleLeafObject(pchilddatanew)){
                  if (!pchilddatanew) 
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(*ppstart &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           *pnresult=0;
                           *piresultcode=iret1;
                           iret=iret1;
                        }
                        if (!*ppstart)
                           *ppstart = pchilddatanew0;
                        else 
                           freeSimpleLeafObjectData(pchilddatanew0);
                        goto IceRoleConnectChildchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pchilddatanew->ObjectName, buf, MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,NULL,extradata);
               }
               *pnresult = i;
               freeSimpleLeafObjectData(*ppstart);
               *ppstart = pchilddatanew0;
               *piresultcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
IceRoleConnectChildchildren:
            UpdRefreshParams(prefreshparms,OT_ICE_GENERIC);
              /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;

     
      case OT_ICE_BUNIT:
         {
            struct IceBusinessUnitsData *pobjdatanew, *pobjdatanew0, *pobjdataold;
            struct IceBusinessUnitsData *ptempold;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;
			switch (iobjecttype) {
				case OT_ICE_BUNIT:
					ppstart       = &(pdata1 ->lpIceBusinessUnitsData);
					prefreshparms = &(pdata1->IceBusinessUnitsRefreshParms);
					pnresult      = &(pdata1->nbicebusinessunits);
					piresultcode  = &(pdata1->icebusinessunitserrcode);
					break;
			}

            if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
               goto IceBusUnitChildren;

            if (bOnlyIfExist && !*ppstart)
               break;

            pobjdataold  = *ppstart;
            pobjdatanew0 = (struct IceBusinessUnitsData * )
                                      ESL_AllocMem(sizeof(struct IceBusinessUnitsData));
            if (!pobjdatanew0)
               return RES_ERR;

            pobjdatanew = pobjdatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++,pobjdatanew=GetNextIceBusinessUnit(pobjdatanew)) {
               /* pgroupdatanew points to zeroed area as the list is new */
               if (!pobjdatanew)
                  iret1 = RES_ERR;
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(*ppstart  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        *pnresult=0;
                        *piresultcode=iret1;
                        iret=iret1;
                     }
                     if (!*ppstart)
                        *ppstart = pobjdatanew0;
                     else 
                        freeIceBusinessUnitsData(pobjdatanew0);
                     goto IceBusUnitChildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pobjdataold && iold<*pnresult) {
                  icmp = x_strcmp(buf,pobjdataold->BusinessunitName);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pobjdatanew->ptemp = pobjdataold;
                     pobjdataold=GetNextIceBusinessUnit(pobjdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pobjdataold=GetNextIceBusinessUnit(pobjdataold);
               }
               fstrncpy(pobjdatanew->BusinessunitName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            *pnresult = i;
            for (i=0,pobjdatanew=pobjdatanew0; i<*pnresult;
                            i++,pobjdatanew=GetNextIceBusinessUnit(pobjdatanew)) {
               if (pobjdatanew->ptemp) {
                  ptemp    = (void *) pobjdatanew->pnext;
                  ptempold =          pobjdatanew->ptemp;
                  memcpy(pobjdatanew,ptempold,sizeof (struct IceBusinessUnitsData));
                  pobjdatanew->pnext = (struct IceBusinessUnitsData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct IceBusinessUnitsData));
                  ptempold->pnext = (struct IceBusinessUnitsData *) ptemp;
               }
            }
            freeIceBusinessUnitsData(*ppstart);
            *ppstart = pobjdatanew0;
            *piresultcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
IceBusUnitChildren:
               UpdRefreshParams(prefreshparms,OT_ICE_GENERIC);
            if (bWithChildren) {
				aparents[0]=NULL; 
				iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_SEC_ROLE,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
				iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_SEC_USER,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
				iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_FACET,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
				iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_PAGE,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
				iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_LOCATION,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
				if (iret1 != RES_SUCCESS)
					iret=iret1;
            }
         }
         break;

      case OT_ICE_BUNIT_FACET          :
      case OT_ICE_BUNIT_PAGE           :
         {
            struct IceBusinessUnitsData     * pbusunitdata;
            struct IceObjectWithRolesAndUsersData * pchilddatanew, * pchilddatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;

            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pbusunitdata= pdata1 -> lpIceBusinessUnitsData;
            if (!pbusunitdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbicebusinessunits;
                        iobj++, pbusunitdata=GetNextIceBusinessUnit(pbusunitdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pbusunitdata->BusinessunitName))
                     continue;
               }
               bParentFound = TRUE;
				switch (iobjecttype) {
					case OT_ICE_BUNIT_FACET:
						ppstart       = &(pbusunitdata->lpObjectFacetsData);
						prefreshparms = &(pbusunitdata->FacetRefreshParms);
						pnresult      = &(pbusunitdata->nbfacets);
						piresultcode  = &(pbusunitdata->facetserrcode);
						break;
					case OT_ICE_BUNIT_PAGE:
						ppstart       = &(pbusunitdata->lpObjectPagesData);
						prefreshparms = &(pbusunitdata->PagesRefreshParms);
						pnresult      = &(pbusunitdata->nbpages);
						piresultcode  = &(pbusunitdata->pageserrcode);
						break;
				}

               if (bOnlyIfExist && !*ppstart)
                  continue;

               if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
                  goto IceBusUnitDocchildren;

               pchilddatanew0 = (struct IceObjectWithRolesAndUsersData * )
                                  ESL_AllocMem(sizeof(struct IceObjectWithRolesAndUsersData));
               if (!pchilddatanew0) 
                  return RES_ERR;

               pchilddatanew = pchilddatanew0;
               aparents[0]=pbusunitdata->BusinessunitName; 
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       NULL,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pchilddatanew=
                                   GetNextObjWithRoleAndUsr(pchilddatanew)){
                  if (!pchilddatanew) 
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(*ppstart &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           *pnresult=0;
                           *piresultcode=iret1;
                           iret=iret1;
                        }
                        if (!*ppstart)
                           *ppstart = pchilddatanew0;
                        else 
                           freeIceObjectWithRolesAndUsrData(pchilddatanew0);
                        goto IceBusUnitDocchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pchilddatanew->ObjectName, buf, MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,NULL,extradata);
               }
               *pnresult = i;
               freeIceObjectWithRolesAndUsrData(*ppstart);
               *ppstart = pchilddatanew0;
               *piresultcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
IceBusUnitDocchildren:
            UpdRefreshParams(prefreshparms,OT_ICE_GENERIC); 
               if (bWithChildren) {
                 aparents[0]=pbusunitdata->BusinessunitName;
                 aparents[1]=NULL; 
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_FACET_ROLE,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_FACET_USER,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_PAGE_ROLE,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_ICE_BUNIT_PAGE_USER,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
               }
           }
           if (!bParentFound)
              myerror(ERR_PARENTNOEXIST);
         }
         break;

      case OT_ICE_BUNIT_FACET_ROLE       :
      case OT_ICE_BUNIT_FACET_USER       :
      case OT_ICE_BUNIT_PAGE_ROLE       :
      case OT_ICE_BUNIT_PAGE_USER       :
         {
            struct IceBusinessUnitsData     * pbusunitdata;
            struct IceObjectWithRolesAndUsersData * pchildlevel1;
            struct SimpleLeafObjectData  *pobjlistnew,*pobjlistnew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bParent2;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;
			int n2;
            if (level!=2)
               return myerror(ERR_LEVELNUMBER);
            pbusunitdata= pdata1 -> lpIceBusinessUnitsData;
            if (!pbusunitdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbicebusinessunits;
                        iobj++, pbusunitdata=GetNextIceBusinessUnit(pbusunitdata)) {
               if (!pbusunitdata)
                  return myerror(ERR_LIST);
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pbusunitdata->BusinessunitName))
                     continue;
               }
               bParentFound=TRUE;
				switch (iobjecttype) {
					case OT_ICE_BUNIT_FACET_ROLE       :
					case OT_ICE_BUNIT_FACET_USER       :
						pchildlevel1=pbusunitdata->lpObjectFacetsData;
						n2=pbusunitdata->nbfacets  ;
						break;
					case OT_ICE_BUNIT_PAGE_ROLE       :
					case OT_ICE_BUNIT_PAGE_USER       :
						pchildlevel1=pbusunitdata->lpObjectPagesData;
						n2=pbusunitdata->nbpages  ;
						break;
				}
               if (!pchildlevel1)  {
                  if (bOnlyIfExist)
                     continue;
                  return myerror(ERR_UPDATEDOMDATA1);
               }
               bParent2 = parentstrings[1]? FALSE: TRUE;
               for (iobj2=0;iobj2<n2;
                            iobj2++,pchildlevel1=GetNextObjWithRoleAndUsr(pchildlevel1)) {
                  if (!pchildlevel1)
                     return myerror(ERR_LIST);
                  if (parentstrings[1]) {
                     if (x_strcmp(parentstrings[1],pchildlevel1->ObjectName))
                        continue;
                  }
                  bParent2 = TRUE;
					switch (iobjecttype) {
						case OT_ICE_BUNIT_FACET_ROLE       :
						case OT_ICE_BUNIT_PAGE_ROLE       :
							ppstart       = &(pchildlevel1->lpObjectRolesData);
							prefreshparms = &(pchildlevel1->ObjectRolesRefreshParms);
							pnresult      = &(pchildlevel1->nbobjectroles);
							piresultcode  = &(pchildlevel1->objectroleserrcode);
							break;
						case OT_ICE_BUNIT_FACET_USER       :
						case OT_ICE_BUNIT_PAGE_USER       :
							ppstart       = &(pchildlevel1->lpObjectUsersData);
							prefreshparms = &(pchildlevel1->ObjectUsersRefreshParms);
							pnresult      = &(pchildlevel1->nbobjectusers);
							piresultcode  = &(pchildlevel1->objectuserserrcode);
							break;
					}
                  if (bOnlyIfExist  && !*ppstart)
                     continue;

                  if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
                     goto busunitdocroleoruserchildren;

                  pobjlistnew0 = (struct SimpleLeafObjectData * )
                                     ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
                  if (!pobjlistnew0) 
                     return RES_ERR;
                  pobjlistnew = pobjlistnew0;
                  aparents[0]=pbusunitdata->BusinessunitName; 
                  aparents[1]=pchildlevel1->ObjectName; 
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
                  for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pobjlistnew=GetNextSimpleLeafObject(pobjlistnew)){
                     if (!pobjlistnew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(*ppstart &&
                                AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
								*pnresult=0;
								*piresultcode=iret1;
								iret=iret1;
                           }
                           if (!*ppstart)
                              *ppstart = pobjlistnew0;
                           else 
                              freeSimpleLeafObjectData(pobjlistnew0);
                           goto busunitdocroleoruserchildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pobjlistnew->ObjectName, buf, MAXOBJECTNAME);
                     pobjlistnew->b1=getint(extradata);
                     pobjlistnew->b2=getint(extradata +  STEPSMALLOBJ );
                     pobjlistnew->b3=getint(extradata + 2*STEPSMALLOBJ);
                     pobjlistnew->b4=getint(extradata + 3*STEPSMALLOBJ);
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
				   *pnresult = i;
				   freeSimpleLeafObjectData(*ppstart);
				   *ppstart = pobjlistnew0;
				   *piresultcode=RES_SUCCESS;
                 if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
busunitdocroleoruserchildren:
                  UpdRefreshParams(prefreshparms,OT_ICE_GENERIC); 
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


      case OT_ICE_BUNIT_SEC_ROLE       :
      case OT_ICE_BUNIT_SEC_USER       :
      case OT_ICE_BUNIT_LOCATION       :
         {
            struct IceBusinessUnitsData     * pbusunitdata;
            struct SimpleLeafObjectData * pchilddatanew, * pchilddatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;

            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pbusunitdata= pdata1 -> lpIceBusinessUnitsData;
            if (!pbusunitdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbicebusinessunits;
                        iobj++, pbusunitdata=GetNextIceBusinessUnit(pbusunitdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pbusunitdata->BusinessunitName))
                     continue;
               }
               bParentFound = TRUE;
				switch (iobjecttype) {
					case OT_ICE_BUNIT_SEC_ROLE:
						ppstart       = &(pbusunitdata->lpObjectRolesData);
						prefreshparms = &(pbusunitdata->ObjectRolesRefreshParms);
						pnresult      = &(pbusunitdata->nbobjectroles);
						piresultcode  = &(pbusunitdata->objectroleserrcode);
						break;
					case OT_ICE_BUNIT_SEC_USER:
						ppstart       = &(pbusunitdata->lpObjectUsersData);
						prefreshparms = &(pbusunitdata->ObjectUsersRefreshParms);
						pnresult      = &(pbusunitdata->nbobjectusers);
						piresultcode  = &(pbusunitdata->objectuserserrcode);
						break;
					case OT_ICE_BUNIT_LOCATION:
						ppstart       = &(pbusunitdata->lpObjectLocationsData);
						prefreshparms = &(pbusunitdata->ObjectLocationsRefreshParms);
						pnresult      = &(pbusunitdata->nbobjectlocations);
						piresultcode  = &(pbusunitdata->objectlocationserrcode);
						break;
				}

               if (bOnlyIfExist && !*ppstart)
                  continue;

               if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
                  goto IceBusUnitChildchildren;

               pchilddatanew0 = (struct SimpleLeafObjectData * )
                                  ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
               if (!pchilddatanew0) 
                  return RES_ERR;

               pchilddatanew = pchilddatanew0;
               aparents[0]=pbusunitdata->BusinessunitName; 
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       NULL,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pchilddatanew=
                                   GetNextSimpleLeafObject(pchilddatanew)){
                  if (!pchilddatanew) 
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(*ppstart &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           *pnresult=0;
                           *piresultcode=iret1;
                           iret=iret1;
                        }
                        if (!*ppstart)
                           *ppstart = pchilddatanew0;
                        else 
                           freeSimpleLeafObjectData(pchilddatanew0);
                        goto IceBusUnitChildchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pchilddatanew->ObjectName, buf, MAXOBJECTNAME);
					switch (iobjecttype) {
						case OT_ICE_BUNIT_SEC_ROLE       :
						case OT_ICE_BUNIT_SEC_USER       :
							pchilddatanew->b1 = getint(extradata);
							pchilddatanew->b2 = getint(extradata +  STEPSMALLOBJ );
							pchilddatanew->b3 = getint(extradata + 2*STEPSMALLOBJ);
							break;
					}
                  iret1 = DBAGetNextObject(buf,NULL,extradata);
               }
               *pnresult = i;
               freeSimpleLeafObjectData(*ppstart);
               *ppstart = pchilddatanew0;
               *piresultcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
IceBusUnitChildchildren:
            UpdRefreshParams(prefreshparms,OT_ICE_GENERIC); 
              /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;

      case OT_ICE_ROLE:
      case OT_ICE_DBUSER:
      case OT_ICE_DBCONNECTION:
      case OT_ICE_SERVER_APPLICATION:
      case OT_ICE_SERVER_LOCATION:
      case OT_ICE_SERVER_VARIABLE:

         {
            struct SimpleLeafObjectData * pobjdatanew, * pobjdatanew0;
			void ** ppstart;
			LPDOMREFRESHPARAMS prefreshparms;
			int *pnresult;
			int *piresultcode;
			switch (iobjecttype) {
				case OT_ICE_ROLE:
					ppstart       = &(pdata1 ->lpIceRolesData);
					prefreshparms = &(pdata1->IceRolesRefreshParms);
					pnresult      = &(pdata1->nbiceroles);
					piresultcode  = &(pdata1->iceroleserrcode);
					break;
				case OT_ICE_DBUSER:
					ppstart       = &(pdata1 ->lpIceDbUsersData);
					prefreshparms = &(pdata1->IceDbUsersRefreshParms);
					pnresult      = &(pdata1->nbicedbusers);
					piresultcode  = &(pdata1->icedbuserserrcode);
					break;
				case OT_ICE_DBCONNECTION:
					ppstart       = &(pdata1 ->lpIceDbConnectionsData);
					prefreshparms = &(pdata1->IceDbConnectRefreshParms);
					pnresult      = &(pdata1->nbicedbconnections);
					piresultcode  = &(pdata1->icedbconnectionserrcode);
					break;
				case OT_ICE_SERVER_APPLICATION:
					ppstart       = &(pdata1 ->lpIceApplicationsData);
					prefreshparms = &(pdata1->IceApplicationRefreshParms);
					pnresult      = &(pdata1->nbiceapplications);
					piresultcode  = &(pdata1->iceapplicationserrcode);
					break;
				case OT_ICE_SERVER_LOCATION:
					ppstart       = &(pdata1 ->lpIceLocationsData);
					prefreshparms = &(pdata1->IceLocationRefreshParms);
					pnresult      = &(pdata1->nbicelocations);
					piresultcode  = &(pdata1->icelocationserrcode);
					break;
				case OT_ICE_SERVER_VARIABLE:
					ppstart       = &(pdata1 ->lpIceVarsData);
					prefreshparms = &(pdata1->IceVarRefreshParms);
					pnresult      = &(pdata1->nbicevars);
					piresultcode  = &(pdata1->icevarserrcode);
					break;
			}

            if (bOnlyIfExist && !*ppstart)
               break;

            if (bUnique && prefreshparms->LastDOMUpdver==DOMUpdver)
               goto IceChildren1;

            pobjdatanew0 = (struct SimpleLeafObjectData * )
                              ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
            if (!pobjdatanew0) 
               return RES_ERR;

            pobjdatanew = pobjdatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0;iret1!=RES_ENDOFDATA;
                 i++,pobjdatanew=GetNextSimpleLeafObject(pobjdatanew)) {
               if (!pobjdatanew)
                  iret1 = RES_ERR;
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!( *ppstart &&
                         AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        *pnresult=0;
                        *piresultcode=iret1;
                        iret=iret1;
                     }
                     if (!*ppstart)
                        *ppstart = pobjdatanew0;
                     else 
                        freeSimpleLeafObjectData(pobjdatanew0);
                     goto IceChildren1;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               fstrncpy(pobjdatanew->ObjectName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            *pnresult = i;
            freeSimpleLeafObjectData(*ppstart);
            *ppstart = pobjdatanew0;
            *piresultcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
IceChildren1:
            UpdRefreshParams(prefreshparms,OT_ICE_GENERIC); 
            if (bWithChildren) {
             /* no children */
            }
         }
         break;

      case OT_ICE_MAIN :
         break;
      case OT_PROFILE      :
         {
            struct ProfileData * pProfiledatanew, * pProfiledatanew0;
            if (bOnlyIfExist && !pdata1->lpProfilesData)
               break;

            if (bUnique && pdata1->ProfilesRefreshParms.LastDOMUpdver==DOMUpdver)
               goto Profilechildren;

            pProfiledatanew0 = (struct ProfileData * )
                              ESL_AllocMem(sizeof(struct ProfileData));
            if (!pProfiledatanew0) 
               return RES_ERR;

            pProfiledatanew = pProfiledatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0;iret1!=RES_ENDOFDATA;
                 i++,pProfiledatanew=GetNextProfile(pProfiledatanew)) {
               if (!pProfiledatanew)
                  iret1 = RES_ERR;
               /* pProfiledatanew points to zeroed area as the list is new */
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!( pdata1->lpProfilesData &&
                         AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbprofiles=0;
                        pdata1->profileserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpProfilesData)
                        pdata1->lpProfilesData = pProfiledatanew0;
                     else 
                        freeProfilesData(pProfiledatanew0);
                     goto Profilechildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               fstrncpy(pProfiledatanew->ProfileName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbprofiles = i;
            freeProfilesData(pdata1->lpProfilesData);
            pdata1->lpProfilesData = pProfiledatanew0;
            pdata1->profileserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
Profilechildren:
            UpdRefreshParams(&(pdata1->ProfilesRefreshParms),OT_PROFILE);
            if (bWithChildren) {
             /* no children */
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
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

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

               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbusers = i;
            freeUsersData(pdata1->lpUsersData);
            pdata1->lpUsersData = puserdatanew0;
            pdata1->userserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
userchildren:
            UpdRefreshParams(&(pdata1->UsersRefreshParms),OT_USER);
            if (bWithChildren) {
             /* no children */
            }
         }
         break;
      case OT_GROUP     :
         {
            struct GroupData *pgroupdatanew, *pgroupdatanew0, *pgroupdataold;
            struct GroupData *ptempold;
            if (bUnique && pdata1->GroupsRefreshParms.LastDOMUpdver==DOMUpdver)
               goto groupchildren;

            if (bOnlyIfExist && !pdata1->lpGroupsData)
               break;

            pgroupdataold  = pdata1->lpGroupsData;
            pgroupdatanew0 = (struct GroupData * )
                                      ESL_AllocMem(sizeof(struct GroupData));
            if (!pgroupdatanew0)
               return RES_ERR;

            pgroupdatanew = pgroupdatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++,pgroupdatanew=GetNextGroup(pgroupdatanew)) {
               /* pgroupdatanew points to zeroed area as the list is new */
               if (!pgroupdatanew)
                  iret1 = RES_ERR;
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpGroupsData  &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbgroups=0;
                        pdata1->groupserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpGroupsData)
                        pdata1->lpGroupsData = pgroupdatanew0;
                     else 
                        freeGroupsData(pgroupdatanew0);
                     goto groupchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (pgroupdataold && iold<pdata1->nbgroups) {
                  icmp = x_strcmp(buf,pgroupdataold->GroupName);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     pgroupdatanew->ptemp = pgroupdataold;
                     pgroupdataold=GetNextGroup(pgroupdataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  pgroupdataold=GetNextGroup(pgroupdataold);
               }
               fstrncpy(pgroupdatanew->GroupName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbgroups = i;
            for (i=0,pgroupdatanew=pgroupdatanew0; i<pdata1->nbgroups;
                            i++,pgroupdatanew=GetNextGroup(pgroupdatanew)) {
               if (pgroupdatanew->ptemp) {
                  ptemp    = (void *) pgroupdatanew->pnext;
                  ptempold =          pgroupdatanew->ptemp;
                  /* pgroupdatanew->GroupName is the same in both structures*/
                  memcpy(pgroupdatanew,ptempold,sizeof (struct GroupData));
                  pgroupdatanew->pnext = (struct GroupData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct GroupData));
                  ptempold->pnext = (struct GroupData *) ptemp;
               }
            }
            freeGroupsData(pdata1->lpGroupsData);
            pdata1->lpGroupsData = pgroupdatanew0;
            pdata1->groupserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
groupchildren:
            UpdRefreshParams(&(pdata1->GroupsRefreshParms),OT_GROUP);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all groups */
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_GROUPUSER,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
            }
         }
         break;
      case OT_GROUPUSER :
         {
            struct GroupData     * pgroupdata;
            struct GroupUserData * pgroupuserdatanew, * pgroupuserdatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pgroupdata= pdata1 -> lpGroupsData;
            if (!pgroupdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbgroups;
                        iobj++, pgroupdata=GetNextGroup(pgroupdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pgroupdata->GroupName))
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pgroupdata->lpGroupUserData)
                  continue;

               if (bUnique && pgroupdata->GroupUsersRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto groupuserchildren;

               pgroupuserdatanew0 = (struct GroupUserData * )
                                  ESL_AllocMem(sizeof(struct GroupUserData));
               if (!pgroupuserdatanew0) 
                  return RES_ERR;

               pgroupuserdatanew = pgroupuserdatanew0;
               aparents[0]=pgroupdata->GroupName; /* for current group */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       NULL,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pgroupuserdatanew=
                                   GetNextGroupUser(pgroupuserdatanew)){
                  if (!pgroupuserdatanew) 
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pgroupdata->lpGroupUserData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pgroupdata->nbgroupusers=0;
                           pgroupdata->groupuserserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pgroupdata->lpGroupUserData)
                           pgroupdata->lpGroupUserData = pgroupuserdatanew0;
                        else 
                           freeGroupUsersData(pgroupuserdatanew0);
                        goto groupuserchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pgroupuserdatanew->GroupUserName, buf, MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,NULL,extradata);
               }
               pgroupdata->nbgroupusers = i;
               freeGroupUsersData(pgroupdata->lpGroupUserData);
               pgroupdata->lpGroupUserData = pgroupuserdatanew0;
               pgroupdata->groupuserserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
groupuserchildren:
               UpdRefreshParams(&(pgroupdata->GroupUsersRefreshParms),
                                OT_GROUPUSER);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;
      case OT_ROLE      :
         {
            struct RoleData * proledatanew, * proledatanew0, * proledataold;
            struct RoleData * ptempold;
            proledataold  = pdata1->lpRolesData;

            if (bOnlyIfExist && !pdata1->lpRolesData)
               break;

            if (bUnique && pdata1->RolesRefreshParms.LastDOMUpdver==DOMUpdver)
               goto rolechildren;

            proledatanew0 = (struct RoleData * )
                              ESL_AllocMem (sizeof(struct RoleData));
            if (!proledatanew0) 
               return RES_ERR;

            proledatanew = proledatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       NULL,extradata);

            for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                     i++,proledatanew=GetNextRole(proledatanew)) {
               if (!proledatanew)
                  iret=RES_ERR;
               /* proledatanew points to zeroed area as the list is new */
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpRolesData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbroles=0;
                        pdata1->roleserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpRolesData)
                        pdata1->lpRolesData = proledatanew0;
                     else 
                        freeRolesData(proledatanew0);
                     goto rolechildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               while (proledataold && iold<pdata1->nbroles) {
                  icmp = x_strcmp(buf,proledataold->RoleName);
                  if (icmp<0) 
                     break;
                  if (!icmp) {
                     proledatanew->ptemp = proledataold;
                     proledataold=GetNextRole(proledataold);/* should be no double*/
                     break;
                  }
                  iold++;
                  proledataold=GetNextRole(proledataold);
               }
               fstrncpy(proledatanew->RoleName,buf, MAXOBJECTNAME);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbroles = i;
            for (i=0,proledatanew=proledatanew0; i<pdata1->nbroles;
                            i++,proledatanew=GetNextRole(proledatanew)) {
               if (proledatanew->ptemp) {
                  ptemp    = (void *) proledatanew->pnext;
                  ptempold =          proledatanew->ptemp;
                  /* proledatanew->roleName is the same in both structures*/
                  memcpy(proledatanew,ptempold,sizeof (struct RoleData));
                  proledatanew->pnext = (struct RoleData *) ptemp;
                  ptemp = (void *) ptempold->pnext;
                  memset(ptempold,'\0',sizeof (struct RoleData));
                  ptempold->pnext = (struct RoleData *) ptemp;
               }
            }
            freeRolesData(pdata1->lpRolesData);
            pdata1->lpRolesData = proledatanew0;
            pdata1->roleserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
rolechildren:
            UpdRefreshParams(&(pdata1->RolesRefreshParms),OT_ROLE);
            if (bWithChildren) {
              aparents[0]=NULL; /* for all roles */
              iret1 = UpdateDOMDataDetail(hnodestruct,OT_ROLEGRANT_USER,1,aparents,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
              if (iret1 != RES_SUCCESS)
                  iret=iret1;
            }
         }
         break;
      case OT_ROLEGRANT_USER :
         {
            struct RoleData     * pRoledata;
            struct RoleGranteeData * pRoleGranteedatanew, * pRoleGranteedatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pRoledata= pdata1 -> lpRolesData;
            if (!pRoledata)  {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }
            for (iobj=0;iobj<pdata1->nbroles;
                        iobj++, pRoledata=GetNextRole(pRoledata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pRoledata->RoleName))
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pRoledata->lpRoleGranteeData)
                  continue;

               if (bUnique && pRoledata->RoleGranteesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto RoleGranteechildren;

               pRoleGranteedatanew0 = (struct RoleGranteeData * )
                                  ESL_AllocMem(sizeof(struct RoleGranteeData));
               if (!pRoleGranteedatanew0) 
                  return RES_ERR;
               pRoleGranteedatanew = pRoleGranteedatanew0;
               aparents[0]=pRoledata->RoleName; /* for current Role */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       NULL,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pRoleGranteedatanew=
                                   GetNextRoleGrantee(pRoleGranteedatanew)){
                  if (!pRoleGranteedatanew) 
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pRoledata->lpRoleGranteeData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pRoledata->nbrolegrantees=0;
                           pRoledata->rolegranteeserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pRoledata->lpRoleGranteeData)
                           pRoledata->lpRoleGranteeData = pRoleGranteedatanew0;
                        else 
                           freeRoleGranteesData(pRoleGranteedatanew0);
                        goto RoleGranteechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pRoleGranteedatanew->RoleGranteeName, buf, MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,NULL,extradata);
               }
               pRoledata->nbrolegrantees = i;
               freeRoleGranteesData(pRoledata->lpRoleGranteeData);
               pRoledata->lpRoleGranteeData = pRoleGranteedatanew0;
               pRoledata->rolegranteeserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
RoleGranteechildren:
               UpdRefreshParams(&(pRoledata->RoleGranteesRefreshParms),
                                OTLL_GRANTEE);
               /* no children */
               iret=iret; /* otherwise comile error */
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);

         }
         break;
      case OT_LOCATION  :
      case OT_LOCATIONWITHUSAGES  :
         {
            struct LocationData * plocationdatanew, *plocationdatanew0;

            if (bOnlyIfExist && !pdata1->lpLocationsData)
               break;

            if (bUnique && pdata1->LocationsRefreshParms.LastDOMUpdver==DOMUpdver)
               goto locationchildren;

            plocationdatanew0 = (struct LocationData * )
                                 ESL_AllocMem(sizeof(struct LocationData));
            if (!plocationdatanew0) 
               return RES_ERR;

            plocationdatanew = plocationdatanew0; 
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       OT_LOCATION,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);

            for (i=0;iret1!=RES_ENDOFDATA;
                     i++,plocationdatanew= GetNextLocation(plocationdatanew)) {
               if (!plocationdatanew)
                  iret1=RES_ERR;
               /* plocationdatanew points to zeroed area as the list is new */
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpLocationsData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nblocations=0;
                        pdata1->locationserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpLocationsData)
                        pdata1->lpLocationsData = plocationdatanew0;
                     else 
                        freeLocationsData(plocationdatanew0);
                     goto locationchildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               fstrncpy(plocationdatanew->LocationName,buf, MAXOBJECTNAME);
               fstrncpy(plocationdatanew->LocationAreaStart,extradata,
                                                            MAXOBJECTNAME);
               memcpy(plocationdatanew->LocationUsages,
                      extradata+MAXOBJECTNAME,
                      sizeof(plocationdatanew->LocationUsages));
               plocationdatanew->iRawPct=getint(buffilter);

               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nblocations = i;
            freeLocationsData(pdata1->lpLocationsData);
            pdata1->lpLocationsData = plocationdatanew0;
            pdata1->locationserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
locationchildren:
            UpdRefreshParams(&(pdata1->LocationsRefreshParms),OT_LOCATION);
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
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
                  {
                     LPUCHAR aparentsloc[2];
                     LPUCHAR parents4disp[MAXPLEVEL];
                     parents4disp[0]=parentstrings[0];
                     parents4disp[1]=buf;
                     aparentsloc[0]=extradata;
                     if (extradata[MAXOBJECTNAME]==ATTACHED_NO) {
                       if (ptabledatanew->ptemp) {
                          DOMQuickRefreshList(ptabledatanew->ptemp,
                                   OT_INDEX,aparentsloc,0,bWithChildren);
                          if (bWithDisplay) {
                             DOMDisableDisplay(hnodestruct, 0);
                             DOMUpdateDisplayData (hnodestruct, OT_INDEX,2,
                                             parents4disp,FALSE,ACT_BKTASK,0L,0);
                             DOMEnableDisplay(hnodestruct, 0);
                          }
                       }
                       else {
                          DOMQuickRefreshList(ptabledatanew,
                                   OT_INDEX,aparentsloc,0,bWithChildren);
                       }
                     }
                     if (extradata[MAXOBJECTNAME+1]==ATTACHED_NO) {
                       if (ptabledatanew->ptemp) {
                         DOMQuickRefreshList(ptabledatanew->ptemp,
                                  OT_INTEGRITY,aparentsloc,0,bWithChildren);
                         if (bWithDisplay) {
                            DOMDisableDisplay(hnodestruct, 0);
                            DOMUpdateDisplayData (hnodestruct, OT_INTEGRITY,2,
                                            parents4disp,FALSE,ACT_BKTASK,0L,0);
                            DOMEnableDisplay(hnodestruct, 0);
                         }
                       }
                       else {
                         DOMQuickRefreshList(ptabledatanew,
                                  OT_INTEGRITY,aparentsloc,0,bWithChildren);
                       }
                     }
                     if (extradata[MAXOBJECTNAME+3]==ATTACHED_NO) {
                       if (ptabledatanew->ptemp) {
                         DOMQuickRefreshList(ptabledatanew->ptemp,
                                  OT_TABLELOCATION,aparentsloc,1,bWithChildren);
                         if (bWithDisplay) {
                            DOMDisableDisplay(hnodestruct, 0);
                            DOMUpdateDisplayData (hnodestruct, OT_TABLELOCATION,2,
                                            parents4disp,FALSE,ACT_BKTASK,0L,0);
                            DOMEnableDisplay(hnodestruct, 0);
                         }
                       }
                       else {
                         DOMQuickRefreshList(ptabledatanew,
                               OT_TABLELOCATION,aparentsloc,1,bWithChildren);
                       }
                     }
                  }
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
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
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
tablechildren:
               UpdRefreshParams(&(pDBdata->TablesRefreshParms),OT_TABLE);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_INTEGRITY,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_COLUMN,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_RULE,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_INDEX,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_TABLELOCATION,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;

      case OT_VIEW      :
         {
            struct DBData   * pDBdata;
            struct ViewData * pviewdatanew ,* pviewdatanew0, * pviewdataold;
            struct ViewData * ptempold;
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
               bParentFound  = TRUE;
               pviewdataold  = pDBdata->lpViewData;

               if (bOnlyIfExist && !pviewdataold)
                  continue;

               if (bUnique && pDBdata->ViewsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto viewchildren;

               pviewdatanew0 = (struct ViewData * )
                                ESL_AllocMem(sizeof(struct ViewData));
               if (!pviewdatanew0) 
                  return RES_ERR;
               pviewdatanew = pviewdatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0,iold=0; iret1!=RES_ENDOFDATA;
                        i++,pviewdatanew=GetNextView(pviewdatanew)) {
                  if (!pviewdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpViewData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbviews=0;
                           pDBdata->viewserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpViewData)
                           pDBdata->lpViewData = pviewdatanew0;
                        else 
                           freeViewsData(pviewdatanew0);
                        goto viewchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  while (pviewdataold && iold<pDBdata->nbviews) {
                     icmp = DOMTreeCmpTableNames(buf,buffilter,
                           pviewdataold->ViewName,pviewdataold->ViewOwner);
                     if (icmp<0) 
                        break;
                     if (!icmp) {
                        pviewdatanew->ptemp = pviewdataold;
                        pviewdataold=GetNextView(pviewdataold);
                                                    /* should be no double*/
                        break;
                     }
                     iold++;
                     pviewdataold=GetNextView(pviewdataold);
                  }
                  fstrncpy(pviewdatanew->ViewName , buf,       MAXOBJECTNAME);
                  fstrncpy(pviewdatanew->ViewOwner, buffilter, MAXOBJECTNAME);
                  pviewdatanew->Tableid=getint(extradata+4);
                  pviewdatanew->ViewStarType=getint(extradata+4+STEPSMALLOBJ);
                  if (extradata[3]=='S' || extradata[3]=='s')
                     pviewdatanew->bSQL=TRUE;
                  else
                     pviewdatanew->bSQL=FALSE;
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbviews = i;
               for (i=0,pviewdatanew=pviewdatanew0; i<pDBdata->nbviews;
                               i++,pviewdatanew=GetNextView(pviewdatanew)) {
                  if (pviewdatanew->ptemp) {
                     ptemp   = (void *) pviewdatanew->pnext;
                     ptempold=          pviewdatanew->ptemp;
                     /* pviewdatanew->ViewOwner is the same in both structs */
                     fstrncpy(buf,pviewdatanew->ViewName,MAXOBJECTNAME);
                     memcpy(pviewdatanew,ptempold,sizeof (struct ViewData));
                     pviewdatanew->pnext = (struct ViewData *) ptemp;
                     fstrncpy(pviewdatanew->ViewName,buf,MAXOBJECTNAME);
                     ptemp = (void *) ptempold->pnext;
                     memset(ptempold,'\0',sizeof (struct ViewData));
                     ptempold->pnext = (struct ViewData *) ptemp;
                  }
               }
               freeViewsData(pDBdata->lpViewData);
               pDBdata->lpViewData = pviewdatanew0;
               pDBdata->viewserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
viewchildren:
               UpdRefreshParams(&(pDBdata->ViewsRefreshParms),OT_VIEW);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_VIEWTABLE,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
                 iret1 = UpdateDOMDataDetail(hnodestruct,OT_VIEWCOLUMN,2,aparents,
                                      bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
                 if (iret1 != RES_SUCCESS)
                     iret = iret1;
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;

      case OT_INTEGRITY :
         {
            struct DBData        * pDBdata;
            struct TableData     * ptabledata;
            struct IntegrityData * pintegritydatanew, * pintegritydatanew0;
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

                  if (bOnlyIfExist && !ptabledata->lpIntegrityData)
                     continue;

                  if (bWithChildren &&
                      ptabledata->iIntegrityDataQuickInfo ==INFOFROMPARENTIMM) {
                     ptabledata->iIntegrityDataQuickInfo =INFOFROMPARENT;
                     continue;
                  }

                  if (bUnique && ptabledata->IntegritiesRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto integritychildren;

                  pintegritydatanew0 = (struct IntegrityData * )
                                  ESL_AllocMem(sizeof(struct IntegrityData));
                  if (!pintegritydatanew0) 
                     return RES_ERR;
                  pintegritydatanew = pintegritydatanew0;
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
                  for (i=0;iret1!=RES_ENDOFDATA; i++,pintegritydatanew=
                                      GetNextIntegrity(pintegritydatanew)){
                     if (!pintegritydatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(ptabledata->lpIntegrityData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                parentstrings))){
                              ptabledata->nbintegrities=0;
                              ptabledata->iIntegrityDataQuickInfo =INFOFROMLL;
                              ptabledata->integritieserrcode=iret1;
                              iret=iret1;
                           }
                           if (!ptabledata->lpIntegrityData)
                              ptabledata->lpIntegrityData=pintegritydatanew0;
                           else 
                              freeIntegritiesData(pintegritydatanew0);
                           goto integritychildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pintegritydatanew->IntegrityName, buf,
                              MAXOBJECTNAME);
                     fstrncpy(pintegritydatanew->IntegrityOwner, buffilter,
                              MAXOBJECTNAME);
                     pintegritydatanew->IntegrityNumber=getint(extradata);
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  ptabledata->nbintegrities  =i;
                  ptabledata->iIntegrityDataQuickInfo = INFOFROMLL;
                  freeIntegritiesData(ptabledata->lpIntegrityData);
                  ptabledata->lpIntegrityData=pintegritydatanew0;
                  ptabledata->integritieserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
integritychildren:
                  UpdRefreshParams(&(ptabledata->IntegritiesRefreshParms),
                                   OT_INTEGRITY);
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
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
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
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  ptabledata->nbcolumns  =i;
                  freeColumnsData(ptabledata->lpColumnData);
                  ptabledata->lpColumnData = pColumndatanew0;
                  ptabledata->columnserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
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
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
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
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  pViewdata->nbcolumns  =i;
                  freeColumnsData(pViewdata->lpColumnData);
                  pViewdata->lpColumnData = pColumndatanew0;
                  pViewdata->columnserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
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
      case OT_RULE :
         {
            struct DBData        * pDBdata;
            struct TableData     * ptabledata;
            struct RuleData      * pruledatanew, * pruledatanew0;
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

                  if (bOnlyIfExist && !ptabledata->lpRuleData)
                     continue;

                  if (bUnique && ptabledata->RulesRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto Rulechildren;

                  pruledatanew0 = (struct RuleData * )
                                   ESL_AllocMem(sizeof(struct RuleData));
                  if (!pruledatanew0) 
                     return RES_ERR;
                  pruledatanew = pruledatanew0;
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
                        i++,pruledatanew=GetNextRule(pruledatanew)){
                     if (!pruledatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(ptabledata->lpRuleData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                 parentstrings))){
                              ptabledata->nbRules=0;
                              ptabledata->ruleserrcode=iret1;
                              iret=iret1;
                           }
                           if (!ptabledata->lpRuleData)
                              ptabledata->lpRuleData = pruledatanew0;
                           else 
                              freeRulesData(pruledatanew0);
                           goto Rulechildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pruledatanew->RuleName,  buf,
                              MAXOBJECTNAME);
                     fstrncpy(pruledatanew->RuleOwner, buffilter,
                              MAXOBJECTNAME);
                     fstrncpy(pruledatanew->RuleProcedure, extradata,
                              MAXOBJECTNAME);
                     fstrncpy(pruledatanew->RuleText, extradata+MAXOBJECTNAME,
                              MAXOBJECTNAME);
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  ptabledata->nbRules = i;
                  freeRulesData(ptabledata->lpRuleData);
                  ptabledata->lpRuleData = pruledatanew0;
                  ptabledata->ruleserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
Rulechildren:
                  UpdRefreshParams(&(ptabledata->RulesRefreshParms),
                                   OT_RULE);
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
      case OT_S_ALARM_SELSUCCESS_USER    :
      case OT_S_ALARM_SELFAILURE_USER    :
      case OT_S_ALARM_DELSUCCESS_USER    :
      case OT_S_ALARM_DELFAILURE_USER    :
      case OT_S_ALARM_INSSUCCESS_USER    :
      case OT_S_ALARM_INSFAILURE_USER    :
      case OT_S_ALARM_UPDSUCCESS_USER    :
      case OT_S_ALARM_UPDFAILURE_USER    :
         return UpdateDOMDataDetail(hnodestruct,OTLL_SECURITYALARM, (level - 1),
                              parentstrings,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         break;
      case OTLL_SECURITYALARM : /* dom2.c (iter1) needs to be modified if */
                                /* security alarms are no more grouped    */
         {
            struct DBData    * pDBdata;
            struct RawSecurityData *pRawSecuritydatanew,*pRawSecuritydatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpRawSecurityData)
                  continue;

               if (bUnique && pDBdata->RawSecuritiesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto RawSecuritychildren;

               pRawSecuritydatanew0 = (struct RawSecurityData * )
                                 ESL_AllocMem(sizeof(struct RawSecurityData));
               if (!pRawSecuritydatanew0) 
                  return RES_ERR;
               pRawSecuritydatanew = pRawSecuritydatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pRawSecuritydatanew=
                                GetNextRawSecurity(pRawSecuritydatanew)) {
                  if (!pRawSecuritydatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR: 
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpRawSecurityData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbRawSecurities=0;
                           pDBdata->rawsecuritieserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpRawSecurityData)
                           pDBdata->lpRawSecurityData = pRawSecuritydatanew0;
                        else 
                           freeRawSecuritiesData(pRawSecuritydatanew0);
                        goto RawSecuritychildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }

                  fstrncpy(pRawSecuritydatanew->RSecurityName,
                           extradata + (MAXOBJECTNAME+2*STEPSMALLOBJ),
                           MAXOBJECTNAMENOSCHEMA);
                  fstrncpy(pRawSecuritydatanew->RSecurityTable, StringWithoutOwner(buf),
                           MAXOBJECTNAME);
                  OwnerFromString(buf,pRawSecuritydatanew->RSecurityTableOwner);
                  fstrncpy(pRawSecuritydatanew->RSecurityDBEvent,buffilter,
                           MAXOBJECTNAME);
                  fstrncpy(pRawSecuritydatanew->RSecurityUser, extradata,
                           MAXOBJECTNAME);
                  pRawSecuritydatanew->RSecurityNumber=
                              getint(extradata+MAXOBJECTNAME);
                  pRawSecuritydatanew->RSecurityType  =
                              getint(extradata+MAXOBJECTNAME+STEPSMALLOBJ);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbRawSecurities = i;
               freeRawSecuritiesData(pDBdata->lpRawSecurityData);
               pDBdata->lpRawSecurityData = pRawSecuritydatanew0;
               pDBdata->rawsecuritieserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
RawSecuritychildren:
               UpdRefreshParams(&(pDBdata->RawSecuritiesRefreshParms),
                                 OTLL_SECURITYALARM);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_S_ALARM_CO_SUCCESS_USER :
      case OT_S_ALARM_CO_FAILURE_USER :
      case OT_S_ALARM_DI_SUCCESS_USER :
      case OT_S_ALARM_DI_FAILURE_USER :
         return UpdateDOMDataDetail(hnodestruct,OTLL_DBSECALARM, (level - 1),
                              parentstrings,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         break;
      case OTLL_DBSECALARM : /* dom2.c (iter1) needs to be modified if */
                             /* security alarms are no more grouped    */
         {
            struct RawSecurityData *pRawDBSecuritydatanew;
            struct RawSecurityData *pRawDBSecuritydatanew0;
            if (level!=0)
               return myerror(ERR_LEVELNUMBER);

            if (bOnlyIfExist && !pdata1->lpRawDBSecurityData)
               break;

            if (bUnique && pdata1->RawDBSecuritiesRefreshParms.LastDOMUpdver==DOMUpdver)
               goto RawDBSecuritychildren;

            pRawDBSecuritydatanew0 = (struct RawSecurityData * )
                              ESL_AllocMem(sizeof(struct RawSecurityData));
            if (!pRawDBSecuritydatanew0) 
               return RES_ERR;
            pRawDBSecuritydatanew = pRawDBSecuritydatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                    iobjecttype,
                                    0,
                                    NULL,
                                    TRUE,
                                    buf,
                                    buffilter,extradata);
            for (i=0;iret1!=RES_ENDOFDATA; i++,pRawDBSecuritydatanew=
                             GetNextRawSecurity(pRawDBSecuritydatanew)) {
               if (!pRawDBSecuritydatanew)
                  iret1=RES_ERR;
               switch (iret1) {
                  case RES_ERR: 
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if(!(pdata1->lpRawDBSecurityData &&
                          AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbRawDBSecurities=0;
                        pdata1->rawdbsecuritieserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpRawDBSecurityData)
                        pdata1->lpRawDBSecurityData = pRawDBSecuritydatanew0;
                     else 
                        freeRawSecuritiesData(pRawDBSecuritydatanew0);
                     goto RawDBSecuritychildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }

               fstrncpy(pRawDBSecuritydatanew->RSecurityName,
                        extradata + (MAXOBJECTNAME+2*STEPSMALLOBJ),
                        MAXOBJECTNAMENOSCHEMA);
               fstrncpy(pRawDBSecuritydatanew->RSecurityTable, StringWithoutOwner(buf),
                        MAXOBJECTNAME);
               OwnerFromString(buf,pRawDBSecuritydatanew->RSecurityTableOwner);
               fstrncpy(pRawDBSecuritydatanew->RSecurityDBEvent,buffilter,
                        MAXOBJECTNAME);
               fstrncpy(pRawDBSecuritydatanew->RSecurityUser, extradata,
                        MAXOBJECTNAME);
               pRawDBSecuritydatanew->RSecurityNumber=
                           getint(extradata+MAXOBJECTNAME);
               pRawDBSecuritydatanew->RSecurityType  =
                           getint(extradata+MAXOBJECTNAME+STEPSMALLOBJ);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbRawDBSecurities = i;
            freeRawSecuritiesData(pdata1->lpRawDBSecurityData);
            pdata1->lpRawDBSecurityData = pRawDBSecuritydatanew0;
            pdata1->rawdbsecuritieserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
RawDBSecuritychildren:
            UpdRefreshParams(&(pdata1->RawDBSecuritiesRefreshParms),
                              OTLL_SECURITYALARM);/*same for all sec alarms*/
            if (bWithChildren) {
             /* no children */
            }
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
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
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
      case OT_TABLELOCATION              :
         {
            struct DBData       * pDBdata;
            struct TableData    * ptabledata;
            struct LocationData * pTableLocationdatanew;
            struct LocationData * pTableLocationdatanew0;
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

                  if (bOnlyIfExist && !ptabledata->lpTableLocationData)
                     continue;

                  if (bWithChildren &&
                      ptabledata->iTableLocDataQuickInfo ==INFOFROMPARENTIMM) {
                     ptabledata->iTableLocDataQuickInfo = INFOFROMPARENT;
                     continue;
                  }

                  if (bUnique && ptabledata->TableLocationsRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto TableLocationchildren;

                  pTableLocationdatanew0 = (struct LocationData * )
                                   ESL_AllocMem(sizeof(struct LocationData));
                  if (!pTableLocationdatanew0) 
                     return RES_ERR;
                  pTableLocationdatanew = pTableLocationdatanew0;
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
                  for (i=0;iret1!=RES_ENDOFDATA; i++,pTableLocationdatanew=
                                      GetNextLocation(pTableLocationdatanew)){
                     if (!pTableLocationdatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(ptabledata->lpTableLocationData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                                  parentstrings))){
                              ptabledata->nbTableLocations=0;
                              ptabledata->iTableLocDataQuickInfo = INFOFROMLL;
                              ptabledata->tablelocationserrcode=iret1;
                              iret=iret1;
                           }
                           if (!ptabledata->lpTableLocationData)
                              ptabledata->lpTableLocationData=pTableLocationdatanew0;
                           else 
                              freeLocationsData(pTableLocationdatanew0);
                           goto TableLocationchildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pTableLocationdatanew->LocationName, buf,
                                                              MAXOBJECTNAME);

                     /* the area of the location is not searched here in */
                     /* because the refresh of locations may not be      */
                     /* synchronized. It is done in domdgetd.c           */

                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  ptabledata->nbTableLocations  =i;
                  ptabledata->iTableLocDataQuickInfo = INFOFROMLL;
                  freeLocationsData(ptabledata->lpTableLocationData);
                  ptabledata->lpTableLocationData = pTableLocationdatanew0;
                  ptabledata->tablelocationserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
TableLocationchildren:
                  UpdRefreshParams(&(ptabledata->TableLocationsRefreshParms),
                                 OT_TABLELOCATION);
                  /* no children */
               }
               if (!bParent2) 
                  return myerror(ERR_PARENTNOEXIST);
            }
            if (!bParentFound) 
               return myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_PROCEDURE                  :
         {
            struct DBData        * pDBdata;
            struct ProcedureData * pProceduredatanew, * pProceduredatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpProcedureData)
                  continue;

               if (bUnique && pDBdata->ProceduresRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto Procedurechildren;

               pProceduredatanew0 = (struct ProcedureData * )
                                  ESL_AllocMem(sizeof(struct ProcedureData));
               if (!pProceduredatanew0) 
                  return RES_ERR;
               pProceduredatanew = pProceduredatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pProceduredatanew=
                                       GetNextProcedure(pProceduredatanew)) {
                  if (!pProceduredatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpProcedureData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbProcedures=0;
                           pDBdata->procedureserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpProcedureData)
                           pDBdata->lpProcedureData = pProceduredatanew0;
                        else 
                           freeProceduresData(pProceduredatanew0);
                        goto Procedurechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pProceduredatanew->ProcedureName, buf,
                           MAXOBJECTNAME);
                  fstrncpy(pProceduredatanew->ProcedureOwner, buffilter,
                           MAXOBJECTNAME);
                  fstrncpy(pProceduredatanew->ProcedureText, extradata,
                           MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbProcedures = i;
               freeProceduresData(pDBdata->lpProcedureData);
               pDBdata->lpProcedureData = pProceduredatanew0;
               pDBdata->procedureserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
Procedurechildren:
               UpdRefreshParams(&(pDBdata->ProceduresRefreshParms),
                                 OT_PROCEDURE);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_SEQUENCE:
         {
            struct DBData        * pDBdata;
            struct SequenceData * pSequencedatanew, * pSequencedatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpSequenceData)
                  continue;

               if (bUnique && pDBdata->SequencesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto Sequencechildren;

               pSequencedatanew0 = (struct SequenceData * )
                                  ESL_AllocMem(sizeof(struct SequenceData));
               if (!pSequencedatanew0) 
                  return RES_ERR;
               pSequencedatanew = pSequencedatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pSequencedatanew=
                                       GetNextSequence(pSequencedatanew)) {
                  if (!pSequencedatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpSequenceData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbSequences=0;
                           pDBdata->sequenceserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpSequenceData)
                           pDBdata->lpSequenceData = pSequencedatanew0;
                        else 
                           freeSequencesData(pSequencedatanew0);
                        goto Sequencechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pSequencedatanew->SequenceName, buf,
                           MAXOBJECTNAME);
                  fstrncpy(pSequencedatanew->SequenceOwner, buffilter,
                           MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbSequences = i;
               freeSequencesData(pDBdata->lpSequenceData);
               pDBdata->lpSequenceData = pSequencedatanew0;
               pDBdata->sequenceserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
Sequencechildren:
               UpdRefreshParams(&(pDBdata->SequencesRefreshParms),
                                 OT_SEQUENCE);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
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
				  if (x_strcmp(GetMonInfoName(hnodestruct, OT_DATABASE, parentstrings, buf1,sizeof(buf1)-1),
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       (LPUCHAR *) &Resdata,
                                       TRUE,
                                       (LPUCHAR) &(pmonreplserverdatanew->MonReplServerDta),
                                       NULL,extradata);
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
						   if (!CompareMonInfo(OT_MON_REPLIC_SERVER,lpnew,lpold)) {
							   *lpnew=*lpold;
							   lpold->DBEhdl = DBEHDL_NOTASSIGNED;
						   }
					   }
				  pmonreplserverdatanew=GetNextMonReplServer(pmonreplserverdatanew);
                  if (!pmonreplserverdatanew) {
                     iret1=RES_ERR;
					 continue;
				  }
				  iret1 = DBAGetNextObject((LPUCHAR) &(pmonreplserverdatanew->MonReplServerDta),NULL,extradata);
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
							  char buf[400];
							  wsprintf(buf, ResourceString(IDS_ERR_MORETH1_REPSERV_DIFFLDB),
											lp1->serverno,
											lp1->RunNode,
											lp1->ParentDataBaseName);
							  PushVDBADlg();
							  TS_MessageBox(TS_GetFocus(), buf, NULL,MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
							  PopVDBADlg();
					   }
				   }
	
				   if (lp1->DBEhdl==DBEHDL_ERROR) {
					    int i1;
						UCHAR buf[1000];
						PushVDBADlg();
						sprintf(buf,ResourceString(IDS_ERR_REPSERV_STATUS_UNAVAILABLE),
									   lp1->serverno, lp1->LocalDBNode,lp1->LocalDBName);
                            
                        i1 = TS_MessageBox(TS_GetFocus(),buf,
                              ResourceString(IDS_REFRESH_REPLICSERVER_STATUS),
                              MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
                        PopVDBADlg();
                        if (i1==IDYES) {
						   push4domg();
						   i1=DBEReplGetEntry(lp1->LocalDBNode,lp1->LocalDBName, &lp1->DBEhdl, lp1->serverno);
						   pop4domg();
						   if (i1==RES_SUCCESS) 
						      lp1->startstatus=REPSVR_UNKNOWNSTATUS;
						   else {
						   	   PushVDBADlg();
							   sprintf(lp1->FullStatus,ResourceString(IDS_ERR_UNABLE2GET_REPSERV_STATUS),
						               lp1->serverno, lp1->LocalDBNode,lp1->LocalDBName);
                               TS_MessageBox(TS_GetFocus(),
                                  lp1->FullStatus,
                                  NULL,
                                  MB_OK|MB_TASKMODAL);
                               PopVDBADlg();
							   lp1->DBEhdl=DBEHDL_ERROR;
							   lp1->startstatus=REPSVR_ERROR;
						   }
						}
						else
							lp1->DBEhdl=DBEHDL_ERROR_ALLWAYS;
				   }
				  
				   if (lp1->DBEhdl==DBEHDL_NOTASSIGNED) {
					    int i1;
						push4domg();
						i1=DBEReplGetEntry(lp1->LocalDBNode,lp1->LocalDBName, &lp1->DBEhdl, lp1->serverno);
						pop4domg();
						if (i1!=RES_SUCCESS) {
							PushVDBADlg();
							sprintf(lp1->FullStatus,ResourceString(IDS_ERR_UNABLE2GET_REPSERV_STATUS),
								    lp1->serverno, lp1->LocalDBNode,lp1->LocalDBName);
                            TS_MessageBox(TS_GetFocus(),
                               lp1->FullStatus,
                               NULL,
                               MB_OK|MB_TASKMODAL);
                            PopVDBADlg();
							lp1->DBEhdl=DBEHDL_ERROR;
							lp1->startstatus=REPSVR_ERROR;
						}
				   }
				}
               freeMonReplServerData(pDBdata->lpMonReplServerData);
               pDBdata->lpMonReplServerData = pmonreplserverdatanew0;
               pDBdata->monreplservererrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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


      case OT_SCHEMAUSER :
         {
            struct DBData         * pDBdata;
            struct SchemaUserData * pSchemaUserdatanew, * pSchemaUserdatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpSchemaUserData)
                  continue;

               if (bUnique && pDBdata->SchemasRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto SchemaUserchildren;

               pSchemaUserdatanew0 = (struct SchemaUserData * )
                                  ESL_AllocMem(sizeof(struct SchemaUserData));
               if (!pSchemaUserdatanew0) 
                  return RES_ERR;
               pSchemaUserdatanew = pSchemaUserdatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pSchemaUserdatanew =
                                      GetNextSchemaUser(pSchemaUserdatanew)){
                  if (!pSchemaUserdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpSchemaUserData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbSchemas=0;
                           pDBdata->schemaserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpSchemaUserData)
                           pDBdata->lpSchemaUserData = pSchemaUserdatanew0;
                        else 
                           freeSchemaUsersData(pSchemaUserdatanew0);
                        goto SchemaUserchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pSchemaUserdatanew->SchemaUserName, buf,
                                                               MAXOBJECTNAME);
                  fstrncpy(pSchemaUserdatanew->SchemaUserOwner,buffilter,
                                                               MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbSchemas = i;
               freeSchemaUsersData(pDBdata->lpSchemaUserData);
               pDBdata->lpSchemaUserData = pSchemaUserdatanew0;
               pDBdata->schemaserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
SchemaUserchildren:
               UpdRefreshParams(&(pDBdata->SchemasRefreshParms),
                                 OT_SCHEMAUSER);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_SYNONYM :
         {
            struct DBData      * pDBdata;
            struct SynonymData * pSynonymdatanew, * pSynonymdatanew0;
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
               bParentFound = TRUE;

               if (bOnlyIfExist && !pDBdata->lpSynonymData)
                  continue;

               if (bUnique && pDBdata->SynonymsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto Synonymchildren;

               pSynonymdatanew0 = (struct SynonymData * )
                                   ESL_AllocMem(sizeof(struct SynonymData));
               if (!pSynonymdatanew0) 
                  return RES_ERR;
               pSynonymdatanew = pSynonymdatanew0;
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pSynonymdatanew=GetNextSynonym(pSynonymdatanew)) {
                  if (!pSynonymdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpSynonymData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbSynonyms=0;
                           pDBdata->synonymserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpSynonymData)
                           pDBdata->lpSynonymData = pSynonymdatanew0;
                        else 
                           freeSynonymsData(pSynonymdatanew0);
                        goto Synonymchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pSynonymdatanew->SynonymName,  buf,
                                                          MAXOBJECTNAME);
                  fstrncpy(pSynonymdatanew->SynonymOwner, buffilter,
                                                          MAXOBJECTNAME);
                  fstrncpy(pSynonymdatanew->SynonymTable, extradata,
                                                          MAXOBJECTNAME);
                                                 /* contains the owner */
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbSynonyms = i;
               freeSynonymsData(pDBdata->lpSynonymData);
               pDBdata->lpSynonymData = pSynonymdatanew0; 
               pDBdata->synonymserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
Synonymchildren:
               UpdRefreshParams(&(pDBdata->SynonymsRefreshParms),
                                 OT_SYNONYM);
               if (bWithChildren) {
                 aparents[0]=pDBdata->DBName;
                 aparents[1]=NULL; 
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
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
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);
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
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbDBEvents = i;
               freeDBEventsData(pDBdata->lpDBEventData);
               pDBdata->lpDBEventData = pDBEventdatanew0;
               pDBdata->dbeventserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
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
      case OT_REPLIC_CONNECTION :
         {
            struct DBData      * pDBdata;
            struct ReplicConnectionData * pReplicConnectiondatanew ;
            struct ReplicConnectionData * pReplicConnectiondatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpReplicConnectionData)
                  continue;

               if (bUnique && pDBdata->ReplicConnectionsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto ReplicConnectionchildren;

               iReplicVersion=GetReplicInstallStatus(hnodestruct,pDBdata->DBName,
                                                                 pDBdata->DBOwner);

               pReplicConnectiondatanew0 = (struct ReplicConnectionData * )
                            ESL_AllocMem(sizeof(struct ReplicConnectionData));
               if (!pReplicConnectiondatanew0) 
                  return RES_ERR;
               pReplicConnectiondatanew = pReplicConnectiondatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               if (iReplicVersion == REPLIC_NOINSTALL)
                  iret1=RES_ERR;
               else { 
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          GetRepObjectType4ll(iobjecttype,iReplicVersion),
                                          1,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
               }
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pReplicConnectiondatanew=GetNextReplicConnection(pReplicConnectiondatanew)){
                  if (!pReplicConnectiondatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpReplicConnectionData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbReplicConnections=0;
                           pDBdata->replicconnectionserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpReplicConnectionData)
                           pDBdata->lpReplicConnectionData = pReplicConnectiondatanew0;
                        else 
                           freeReplicConnectionsData(pReplicConnectiondatanew0);
                        goto ReplicConnectionchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pReplicConnectiondatanew->NodeName,  buf,
                                                      MAXOBJECTNAME);
                  fstrncpy(pReplicConnectiondatanew->DBName, buffilter,
                                                      MAXOBJECTNAME);
                  pReplicConnectiondatanew->DBNumber    =
                                           getint(extradata);
                  pReplicConnectiondatanew->ReplicVersion=iReplicVersion;
                  switch (iReplicVersion) {
                     case REPLIC_V10:
                     case REPLIC_V105:
                        pReplicConnectiondatanew->ServerNumber=
                                                 getint(extradata+STEPSMALLOBJ);
                        pReplicConnectiondatanew->TargetType  =
                                                 getint(extradata+(2*STEPSMALLOBJ));
                        break;
                     case REPLIC_V11:
                        fstrncpy(pReplicConnectiondatanew->DBMSType,
                                 extradata+(3*STEPSMALLOBJ),
                                 (MAXOBJECTNAME/2));
                        //fstrncpy(pReplicConnectiondatanew->DBMSType,
                        //         extradata+(3*STEPSMALLOBJ),
                        //         (MAXOBJECTNAME/2));
                        break;
                     default:
                        myerror(ERR_REPLICTYPE);
                        break;
                  }
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbReplicConnections = i;
               freeReplicConnectionsData(pDBdata->lpReplicConnectionData);
               pDBdata->lpReplicConnectionData = pReplicConnectiondatanew0;
               pDBdata->replicconnectionserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
ReplicConnectionchildren:
               UpdRefreshParams(&(pDBdata->ReplicConnectionsRefreshParms),
                                 OT_REPLIC_CONNECTION);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_REPLIC_REGTABLE :
         {
            struct DBData      * pDBdata;
            struct ReplicRegTableData * pReplicRegTabledatanew ;
            struct ReplicRegTableData * pReplicRegTabledatanew0;
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
               fstrncpy(bufparent1WithOwner,pDBdata->DBOwner,MAXOBJECTNAME);

               if (bOnlyIfExist && !pDBdata->lpReplicRegTableData)
                  continue;

               if (bUnique && pDBdata->ReplicRegTablesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto ReplicRegTablechildren;

               iReplicVersion=GetReplicInstallStatus(hnodestruct,pDBdata->DBName,
                                                                 pDBdata->DBOwner);

               pReplicRegTabledatanew0 = (struct ReplicRegTableData * )
                            ESL_AllocMem(sizeof(struct ReplicRegTableData));
               if (!pReplicRegTabledatanew0) 
                  return RES_ERR;
               pReplicRegTabledatanew = pReplicRegTabledatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               if (iReplicVersion == REPLIC_NOINSTALL)
                  iret1=RES_ERR;
               else { 
                    iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                            GetRepObjectType4ll(iobjecttype,iReplicVersion),
                                            1,
                                            aparents,
                                            TRUE,
                                            buf,
                                            buffilter,extradata);
               }
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pReplicRegTabledatanew=GetNextReplicRegTable(pReplicRegTabledatanew)){
                  if (!pReplicRegTabledatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpReplicRegTableData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbReplicRegTables=0;
                           pDBdata->replicregtableserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpReplicRegTableData)
                           pDBdata->lpReplicRegTableData = pReplicRegTabledatanew0;
                        else 
                           freeReplicRegTablesData(pReplicRegTabledatanew0);
                        goto ReplicRegTablechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pReplicRegTabledatanew->TableName,  buf,
                                                      MAXOBJECTNAME);
                  if (iReplicVersion==REPLIC_V11)
                     fstrncpy(pReplicRegTabledatanew->TableOwner,
                              buffilter, MAXOBJECTNAME);
                  else
                     fstrncpy(pReplicRegTabledatanew->TableOwner,
                              pDBdata->DBOwner, MAXOBJECTNAME);
                  pReplicRegTabledatanew->CDDSNo    =
                                           getint(extradata);
                  pReplicRegTabledatanew->bTablesCreated=
                                           getint(extradata+STEPSMALLOBJ);
                  pReplicRegTabledatanew->bColumnsRegistered=
                                           getint(extradata+(2*STEPSMALLOBJ));
                  pReplicRegTabledatanew->bRulesCreated=
                                           getint(extradata+(3*STEPSMALLOBJ));
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbReplicRegTables = i;
               freeReplicRegTablesData(pDBdata->lpReplicRegTableData);
               pDBdata->lpReplicRegTableData = pReplicRegTabledatanew0;
               pDBdata->replicregtableserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
ReplicRegTablechildren:
               UpdRefreshParams(&(pDBdata->ReplicRegTablesRefreshParms),
                                 OT_REPLIC_CONNECTION);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_REPLIC_CDDS_DETAIL:
          level = 1;
          iobjecttype =OTLL_REPLIC_CDDS;
      case OT_REPLIC_CDDS :
      case OTLL_REPLIC_CDDS :
         {
            struct DBData      * pDBdata;
            struct RawReplicCDDSData * pRawReplicCDDSdatanew ;
            struct RawReplicCDDSData * pRawReplicCDDSdatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpRawReplicCDDSData)
                  continue;

               if (bUnique && pDBdata->RawReplicCDDSRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto RawReplicCDDSchildren;

               iReplicVersion=GetReplicInstallStatus(hnodestruct,pDBdata->DBName,
                                                                 pDBdata->DBOwner);

               pRawReplicCDDSdatanew0 = (struct RawReplicCDDSData * )
                            ESL_AllocMem(sizeof(struct RawReplicCDDSData));
               if (!pRawReplicCDDSdatanew0) 
                  return RES_ERR;
               pRawReplicCDDSdatanew = pRawReplicCDDSdatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               if (iReplicVersion == REPLIC_NOINSTALL)
                  iret1=RES_ERR;
               else {
                    iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                            GetRepObjectType4ll(iobjecttype,iReplicVersion),
                                            1,
                                            aparents,
                                            TRUE,
                                            buf,
                                            buffilter,extradata);
               }
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pRawReplicCDDSdatanew=GetNextRawReplicCDDS(pRawReplicCDDSdatanew)){
                  if (!pRawReplicCDDSdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpRawReplicCDDSData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbRawReplicCDDS=0;
                           pDBdata->rawrepliccddserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpRawReplicCDDSData)
                           pDBdata->lpRawReplicCDDSData = pRawReplicCDDSdatanew0;
                        else 
                           freeRawReplicCDDSData(pRawReplicCDDSdatanew0);
                        goto RawReplicCDDSchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  pRawReplicCDDSdatanew->CDDSNo    =
                                           getint(extradata);
                  pRawReplicCDDSdatanew->LocalDB   =
                                           getint(extradata+STEPSMALLOBJ);
                  pRawReplicCDDSdatanew->SourceDB  =
                                           getint(extradata+(2*STEPSMALLOBJ));
                  pRawReplicCDDSdatanew->TargetDB  =
                                           getint(extradata+(3*STEPSMALLOBJ));
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbRawReplicCDDS = i;
               freeRawReplicCDDSData(pDBdata->lpRawReplicCDDSData);
               pDBdata->lpRawReplicCDDSData = pRawReplicCDDSdatanew0;
               pDBdata->rawrepliccddserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
RawReplicCDDSchildren:
               UpdRefreshParams(&(pDBdata->RawReplicCDDSRefreshParms),
                                 OT_REPLIC_CONNECTION);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_REPLIC_MAILUSER :
         {
            struct DBData      * pDBdata;
            struct ReplicMailErrData * pReplicMailErrdatanew ;
            struct ReplicMailErrData * pReplicMailErrdatanew0;
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

               if (bOnlyIfExist && !pDBdata->lpReplicMailErrData)
                  continue;

               if (bUnique && pDBdata->ReplicMailErrsRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto ReplicMailErrchildren;

               iReplicVersion=GetReplicInstallStatus(hnodestruct,pDBdata->DBName,
                                                                 pDBdata->DBOwner);

               pReplicMailErrdatanew0 = (struct ReplicMailErrData * )
                            ESL_AllocMem(sizeof(struct ReplicMailErrData));
               if (!pReplicMailErrdatanew0) 
                  return RES_ERR;
               pReplicMailErrdatanew = pReplicMailErrdatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               if (iReplicVersion == REPLIC_NOINSTALL)
                  iret1=RES_ERR;
               else { 
                    iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                            GetRepObjectType4ll(iobjecttype,iReplicVersion),
                                            1,
                                            aparents,
                                            TRUE,
                                            buf,
                                            buffilter,extradata);
               }
               for (i=0;iret1!=RES_ENDOFDATA;
                        i++,pReplicMailErrdatanew=GetNextReplicMailErr(pReplicMailErrdatanew)){
                  if (!pReplicMailErrdatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR:
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpReplicMailErrData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbReplicMailErrs=0;
                           pDBdata->replicmailerrserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpReplicMailErrData)
                           pDBdata->lpReplicMailErrData = pReplicMailErrdatanew0;
                        else 
                           freeReplicMailErrsData(pReplicMailErrdatanew0);
                        goto ReplicMailErrchildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pReplicMailErrdatanew->UserName,  buf,
                                                      MAXOBJECTNAME);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbReplicMailErrs = i;
               freeReplicMailErrsData(pDBdata->lpReplicMailErrData);
               pDBdata->lpReplicMailErrData = pReplicMailErrdatanew0;
               pDBdata->replicmailerrserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
ReplicMailErrchildren:
               UpdRefreshParams(&(pDBdata->ReplicMailErrsRefreshParms),
                                 OT_REPLIC_CONNECTION);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;
      case OT_VIEWTABLE :
         {
            struct DBData          * pDBdata;
            struct ViewData        * pViewdata;
            struct ViewTableData   * pViewTabledatanew, * pViewTabledatanew0;
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
               if (!pViewdata) { 
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

                  if (bOnlyIfExist && !pViewdata->lpViewTableData)
                     continue;

                  if (bUnique && pViewdata->ViewTablesRefreshParms.LastDOMUpdver==DOMUpdver)
                     goto ViewTablechildren;

                  pViewTabledatanew0 = (struct ViewTableData * )
                                   ESL_AllocMem(sizeof(struct ViewTableData));
                  if (!pViewTabledatanew0) 
                     return RES_ERR;
                  pViewTabledatanew = pViewTabledatanew0; 
                  aparents[0]=pDBdata->DBName; 
                  aparents[1]=StringWithOwner(pViewdata->ViewName,
                                pViewdata->ViewOwner,bufparent1WithOwner); 
                  iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                          iobjecttype,
                                          2,
                                          aparents,
                                          TRUE,
                                          buf,
                                          buffilter,extradata);
                  for (i=0;iret1!=RES_ENDOFDATA; i++,pViewTabledatanew=
                                        GetNextViewTable(pViewTabledatanew)){
                     if (!pViewTabledatanew)
                        iret1=RES_ERR;
                     switch (iret1) {
                        case RES_ERR:
                        case RES_NOGRANT:
                        case RES_TIMEOUT:
                           if(!(pViewdata->lpViewTableData &&
                                AskIfKeepOldState(iobjecttype,iret1,
                                                  parentstrings))){
                              pViewdata->nbViewTables=0;
                              pViewdata->viewtableserrcode=iret1;
                              iret=iret1;
                           }
                           if (!pViewdata->lpViewTableData)
                              pViewdata->lpViewTableData = pViewTabledatanew0;
                           else 
                              freeViewTablesData(pViewTabledatanew0);
                           goto ViewTablechildren;
                           break;
                        case RES_ENDOFDATA:
                           continue;
                           break;
                     }
                     fstrncpy(pViewTabledatanew->TableName,buf,MAXOBJECTNAME);
                     fstrncpy(pViewTabledatanew->TableOwner,buffilter,MAXOBJECTNAME);
                     iret1 = DBAGetNextObject(buf,buffilter,extradata);
                  }
                  pViewdata->nbViewTables  =i;
                  freeViewTablesData(pViewdata->lpViewTableData);
                  pViewdata->lpViewTableData = pViewTabledatanew0;
                  pViewdata->viewtableserrcode=RES_SUCCESS;
                  if (bWithDisplay) {
                     DOMDisableDisplay(hnodestruct, 0);
                     DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                           FALSE,ACT_BKTASK,0L,0);
                     DOMEnableDisplay(hnodestruct, 0);
                  }
ViewTablechildren:
                  UpdRefreshParams(&(pViewdata->ViewTablesRefreshParms),
                                 OT_VIEWTABLE);
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

         return UpdateDOMDataDetail(hnodestruct,OTLL_DBGRANTEE, (level - 1),
            parentstrings,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         break;
      case OTLL_OIDTDBGRANTEE:
         {
            struct DBData         * pDBdata;
            struct RawDBGranteeData * pRawDBGranteedatanew, * pRawDBGranteedatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bGetSystem;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }

            bGetSystem   = bWithSystem || pDBdata->HasSystemOIDTDBGrantees ?
                           TRUE:FALSE;

            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pDBdata->lpOIDTDBGranteeData)
                  continue;

               if (bUnique && pDBdata->OIDTDBGranteeRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto RawOIDTDBGranteechildren;

               pRawDBGranteedatanew0 = (struct RawDBGranteeData * )
                                 ESL_AllocMem(sizeof(struct RawDBGranteeData));
               if (!pRawDBGranteedatanew0) 
                  return RES_ERR;
               pRawDBGranteedatanew = pRawDBGranteedatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       bGetSystem,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pRawDBGranteedatanew=
                                     GetNextRawDBGrantee(pRawDBGranteedatanew)){
                  if (!pRawDBGranteedatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR: 
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpOIDTDBGranteeData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbOIDTDBGrantees    =0;
                           pDBdata->OIDTDBGranteeerrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpOIDTDBGranteeData)
                           pDBdata->lpOIDTDBGranteeData = pRawDBGranteedatanew0;
                        else 
                           freeRawDBGranteesData(pRawDBGranteedatanew0);
                        goto RawOIDTDBGranteechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pRawDBGranteedatanew->Grantee, buf,
                                                          MAXOBJECTNAME);
                  fstrncpy(pRawDBGranteedatanew->Database,buffilter,
                                                          MAXOBJECTNAME);
                  pRawDBGranteedatanew->GrantType = getint(extradata);
                  pRawDBGranteedatanew->GrantExtraInt =
                                  getint(extradata+STEPSMALLOBJ);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbOIDTDBGrantees = i;
               pDBdata->HasSystemOIDTDBGrantees=bGetSystem  ;
               freeRawDBGranteesData(pDBdata->lpOIDTDBGranteeData);
               pDBdata->lpOIDTDBGranteeData = pRawDBGranteedatanew0;
               pDBdata->OIDTDBGranteeerrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
RawOIDTDBGranteechildren:
               UpdRefreshParams(&(pDBdata->OIDTDBGranteeRefreshParms),
                              OTLL_DBGRANTEE);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
          }
          break;
      case OTLL_DBGRANTEE:
         {
            struct RawDBGranteeData * pRawDBGranteedatanew;
            struct RawDBGranteeData * pRawDBGranteedatanew0;

            if (bOnlyIfExist && !pdata1->lpRawDBGranteeData)
               break;

            if (bUnique && pdata1->RawDBGranteesRefreshParms.LastDOMUpdver==DOMUpdver)
               goto RawDBGranteechildren;

            pRawDBGranteedatanew0 = (struct RawDBGranteeData * )
                                ESL_AllocMem(sizeof(struct RawDBGranteeData));
            if (!pRawDBGranteedatanew0) 
               return RES_ERR;

            pRawDBGranteedatanew = pRawDBGranteedatanew0;
            iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       OTLL_DBGRANTEE,
                                       0,
                                       NULL,
                                       TRUE,
                                       buf,
                                       buffilter,extradata);

            for (i=0;iret1!=RES_ENDOFDATA; i++,pRawDBGranteedatanew=
                                   GetNextRawDBGrantee(pRawDBGranteedatanew)){
               if (!pRawDBGranteedatanew)
                  iret1=RES_ERR;
               switch (iret1) {
                  case RES_ERR:
                  case RES_NOGRANT:
                  case RES_TIMEOUT:
                     if (!(pdata1->lpRawDBGranteeData &&
                           AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                        pdata1->nbRawDBGrantees=0;
                        pdata1->Rawdbgranteeserrcode=iret1;
                        iret=iret1;
                     }
                     if (!pdata1->lpRawDBGranteeData)
                        pdata1->lpRawDBGranteeData = pRawDBGranteedatanew0;
                     else 
                        freeRawDBGranteesData(pRawDBGranteedatanew0);
                     goto RawDBGranteechildren;
                     break;
                  case RES_ENDOFDATA:
                     continue;
                     break;
               }
               fstrncpy(pRawDBGranteedatanew->Grantee, buf,
                                                       MAXOBJECTNAME);
               fstrncpy(pRawDBGranteedatanew->Database,buffilter,
                                                       MAXOBJECTNAME);
               pRawDBGranteedatanew->GrantType = getint(extradata);
               pRawDBGranteedatanew->GrantExtraInt =
                                  getint(extradata+STEPSMALLOBJ);
               iret1 = DBAGetNextObject(buf,buffilter,extradata);
            }
            pdata1->nbRawDBGrantees = i;
            freeRawDBGranteesData(pdata1->lpRawDBGranteeData);
            pdata1->lpRawDBGranteeData = pRawDBGranteedatanew0;
            pdata1->Rawdbgranteeserrcode=RES_SUCCESS;
            if (bWithDisplay) {
               DOMDisableDisplay(hnodestruct, 0);
               DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                     FALSE,ACT_BKTASK,0L,0);
               DOMEnableDisplay(hnodestruct, 0);
            }
RawDBGranteechildren:
            UpdRefreshParams(&(pdata1->RawDBGranteesRefreshParms),
                              OTLL_DBGRANTEE);
            if (bWithChildren) {
             /* no children */
            }
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
         return UpdateDOMDataDetail(hnodestruct,OTLL_GRANTEE, (level - 1),
                              parentstrings,bWithSystem,bWithChildren,bOnlyIfExist,bUnique,bWithDisplay);
         break;
      case OTLL_GRANTEE :
         {
            struct DBData         * pDBdata;
            struct RawGranteeData * pRawGranteedatanew, * pRawGranteedatanew0;
            BOOL bParentFound = parentstrings[0]? FALSE: TRUE;
            BOOL bGetSystem;
            if (level!=1)
               return myerror(ERR_LEVELNUMBER);
            pDBdata= pdata1 -> lpDBData;
            if (!pDBdata) {
               if (bOnlyIfExist)
                 break;
               return myerror(ERR_UPDATEDOMDATA1);
            }

            bGetSystem   = bWithSystem || pDBdata->HasSystemRawGrantees ?
                           TRUE:FALSE;

            for (iobj=0;iobj<pdata1->nbdb;
                        iobj++,pDBdata=GetNextDB(pDBdata)) {
               if (parentstrings[0]) {
                  if (x_strcmp(parentstrings[0],pDBdata->DBName))
                     continue;
               }
               bParentFound = TRUE;

               if (bOnlyIfExist && !pDBdata->lpRawGranteeData)
                  continue;

               if (bUnique && pDBdata->RawGranteesRefreshParms.LastDOMUpdver==DOMUpdver)
                  goto RawGranteechildren;

               pRawGranteedatanew0 = (struct RawGranteeData * )
                                 ESL_AllocMem(sizeof(struct RawGranteeData));
               if (!pRawGranteedatanew0) 
                  return RES_ERR;
               pRawGranteedatanew = pRawGranteedatanew0; 
               aparents[0]=pDBdata->DBName; /* for current database */
               iret1 = DBAGetFirstObject (pdata1->VirtNodeName,
                                       iobjecttype,
                                       1,
                                       aparents,
                                       bGetSystem,
                                       buf,
                                       buffilter,extradata);
               for (i=0;iret1!=RES_ENDOFDATA; i++,pRawGranteedatanew=
                                     GetNextRawGrantee(pRawGranteedatanew)){
                  if (!pRawGranteedatanew)
                     iret1=RES_ERR;
                  switch (iret1) {
                     case RES_ERR: 
                     case RES_NOGRANT:
                     case RES_TIMEOUT:
                        if(!(pDBdata->lpRawGranteeData &&
                             AskIfKeepOldState(iobjecttype,iret1,parentstrings))){
                           pDBdata->nbRawGrantees=0;
                           pDBdata->rawgranteeserrcode=iret1;
                           iret=iret1;
                        }
                        if (!pDBdata->lpRawGranteeData)
                           pDBdata->lpRawGranteeData = pRawGranteedatanew0;
                        else 
                           freeRawGranteesData(pRawGranteedatanew0);
                        goto RawGranteechildren;
                        break;
                     case RES_ENDOFDATA:
                        continue;
                        break;
                  }
                  fstrncpy(pRawGranteedatanew->GranteeObject, buf,
                                                        MAXOBJECTNAME);
                  fstrncpy(pRawGranteedatanew->GranteeObjectOwner, buffilter,
                                                        MAXOBJECTNAME);
                  fstrncpy(pRawGranteedatanew->Grantee, extradata,
                                                        MAXOBJECTNAME);
                  pRawGranteedatanew->GranteeObjectType=
                              getint(extradata+MAXOBJECTNAME);
                  pRawGranteedatanew->GrantType  =
                              getint(extradata+MAXOBJECTNAME+STEPSMALLOBJ);
                  iret1 = DBAGetNextObject(buf,buffilter,extradata);
               }
               pDBdata->nbRawGrantees = i;
               pDBdata->HasSystemRawGrantees=bGetSystem  ;
               freeRawGranteesData(pDBdata->lpRawGranteeData);
               pDBdata->lpRawGranteeData = pRawGranteedatanew0;
               pDBdata->rawgranteeserrcode=RES_SUCCESS;
               if (bWithDisplay) {
                  DOMDisableDisplay(hnodestruct, 0);
                  DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                                        FALSE,ACT_BKTASK,0L,0);
                  DOMEnableDisplay(hnodestruct, 0);
               }
RawGranteechildren:
               UpdRefreshParams(&(pDBdata->RawGranteesRefreshParms),
                              OTLL_GRANTEE);
               if (bWithChildren) {
                  /* no children */
               }
            }
            if (!bParentFound)
               myerror(ERR_PARENTNOEXIST);
         }
         break;

      /*** cross-references ***/

      case OTR_PROC_RULE          :
        iret1=UpdateDOMDataDetail(hnodestruct,OT_TABLE,1,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=parentstrings[0];
        aparents[1]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_RULE,2,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

      case OTR_LOCATIONTABLE      :  
        iret1=UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_TABLE,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        aparents[1]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_TABLELOCATION,2,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

      case OTR_TABLEVIEW          :  
        iret1=UpdateDOMDataDetail(hnodestruct,OT_VIEW,1,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=parentstrings[0];
        aparents[1]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_VIEWTABLE,2,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

      case OTR_USERGROUP          :  
        iret1=UpdateDOMDataDetail(hnodestruct,OT_GROUP,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_GROUPUSER,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;
      case OTR_USERSCHEMA         :  
        iret1=UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_SCHEMAUSER,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }

      case OTR_INDEXSYNONYM       :  
      case OTR_TABLESYNONYM       :  
      case OTR_VIEWSYNONYM        :  
        iret1=UpdateDOMDataDetail(hnodestruct,OT_SYNONYM,1,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;
      case OTR_DBGRANT_ACCESY_DB  :  
      case OTR_DBGRANT_ACCESN_DB  :  
      case OTR_DBGRANT_CREPRY_DB  :  
      case OTR_DBGRANT_CREPRN_DB  :  
      case OTR_DBGRANT_SEQCRY_DB  :  
      case OTR_DBGRANT_SEQCRN_DB  :  
      case OTR_DBGRANT_CRETBY_DB  :  
      case OTR_DBGRANT_CRETBN_DB  :  
      case OTR_DBGRANT_DBADMY_DB  :  
      case OTR_DBGRANT_DBADMN_DB  :  
      case OTR_DBGRANT_LKMODY_DB  :  
      case OTR_DBGRANT_LKMODN_DB  :  
      case OTR_DBGRANT_QRYIOY_DB  :  
      case OTR_DBGRANT_QRYION_DB  :  
      case OTR_DBGRANT_QRYRWY_DB  :  
      case OTR_DBGRANT_QRYRWN_DB  :  
      case OTR_DBGRANT_UPDSCY_DB  :  
      case OTR_DBGRANT_UPDSCN_DB  :   
      case OTR_DBGRANT_SELSCY_DB  : 
      case OTR_DBGRANT_SELSCN_DB  : 
      case OTR_DBGRANT_CNCTLY_DB  : 
      case OTR_DBGRANT_CNCTLN_DB  : 
      case OTR_DBGRANT_IDLTLY_DB  : 
      case OTR_DBGRANT_IDLTLN_DB  : 
      case OTR_DBGRANT_SESPRY_DB  : 
      case OTR_DBGRANT_SESPRN_DB  : 
      case OTR_DBGRANT_TBLSTY_DB  : 
      case OTR_DBGRANT_TBLSTN_DB  : 
      case OTR_DBGRANT_QRYCPN_DB  :
      case OTR_DBGRANT_QRYCPY_DB  :
      case OTR_DBGRANT_QRYPGN_DB  :
      case OTR_DBGRANT_QRYPGY_DB  :
      case OTR_DBGRANT_QRYCON_DB  :
      case OTR_DBGRANT_QRYCOY_DB  :
        iret1=UpdateDOMDataDetail(hnodestruct,OTLL_DBGRANTEE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;
      case OTR_GRANTEE_SEL_TABLE  :  
      case OTR_GRANTEE_INS_TABLE  :  
      case OTR_GRANTEE_UPD_TABLE  :  
      case OTR_GRANTEE_DEL_TABLE  :  
      case OTR_GRANTEE_REF_TABLE  :  
      case OTR_GRANTEE_CPI_TABLE  :
      case OTR_GRANTEE_CPF_TABLE  :
      case OTR_GRANTEE_ALL_TABLE  :  
      case OTR_GRANTEE_SEL_VIEW   :  
      case OTR_GRANTEE_INS_VIEW   :  
      case OTR_GRANTEE_UPD_VIEW   :  
      case OTR_GRANTEE_DEL_VIEW   :  
      case OTR_GRANTEE_RAISE_DBEVENT :
      case OTR_GRANTEE_REGTR_DBEVENT :
      case OTR_GRANTEE_EXEC_PROC     :
      case OTR_GRANTEE_NEXT_SEQU     :
        iret1=UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OTLL_GRANTEE,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

      case OTR_GRANTEE_ROLE          :
        iret1=UpdateDOMDataDetail(hnodestruct,OT_ROLE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OT_ROLEGRANT_USER,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

      case OTR_ALARM_SELSUCCESS_TABLE : 
      case OTR_ALARM_SELFAILURE_TABLE : 
      case OTR_ALARM_DELSUCCESS_TABLE : 
      case OTR_ALARM_DELFAILURE_TABLE : 
      case OTR_ALARM_INSSUCCESS_TABLE : 
      case OTR_ALARM_INSFAILURE_TABLE :
      case OTR_ALARM_UPDSUCCESS_TABLE :
      case OTR_ALARM_UPDFAILURE_TABLE :

        iret1=UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        aparents[0]=NULL;
        iret1=UpdateDOMDataDetail(hnodestruct,OTLL_SECURITYALARM,1,aparents,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;

                                      
      case OTR_REPLIC_CDDS_TABLE      :
         break;
      case OTR_REPLIC_TABLE_CDDS      :
        iret1=UpdateDOMDataDetail(hnodestruct,OT_REPLIC_REGTABLE,1,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;
      case OT_RULEPROC :
         // to be finished: the caller should call this function for OT_RULE
         return RES_SUCCESS;
      case OTR_CDB:
        // noifr01/schph01 27/09/99 (bug #98837)
        // force refresh was missing for STAR coordinator databases branches.
        // Since it is a cross-reference branch (OTR_xx), refresh the base branch
        // information it is derived from, i.e. OT_DATABASE in this particular case
        iret1=UpdateDOMDataDetail(hnodestruct,OT_DATABASE,0,parentstrings,
                        bWithSystem,FALSE,bOnlyIfExist,bUnique,
                        bWithDisplay);
        if (iret1!=RES_SUCCESS) {
           iret=iret1;
           break;
        }
        break;
      default:
         {
            char toto[40];
            // type of error %d
            wsprintf(toto, ResourceString ((UINT)IDS_E_TYPEOF_ERROR), iobjecttype);
            TS_MessageBox( NULL, toto, NULL, MB_OK | MB_TASKMODAL);
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
   return UpdateDOMDataDetail(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,bWithChildren,FALSE,FALSE,FALSE);
}

int UpdateReplStatusinCache(LPUCHAR NodeName,LPUCHAR DBName,int svrno,LPUCHAR evttext,BOOL bDisplay)
{
   int hdl;
   for (hdl=0;hdl<imaxnodes;hdl++) {
	  struct  ServConnectData * pdata1 = virtnode[hdl].pdata;
      if (pdata1) {
         struct DBData            * pDBdata = pdata1 -> lpDBData;
         struct MonReplServerData * pmonreplserverdata;
		 BOOL b2Refresh=FALSE;
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
		 if (b2Refresh && bDisplay) 
			  UpdateMonDisplay(hdl, 0,NULL,OT_MON_ALL);
      }
   }
   return RES_SUCCESS;
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
   x_strcpy(LocalNode,ResourceString((UINT)IDS_I_LOCALNODE));
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

int CloseCachesExtReplicInfo(LPUCHAR NodeName)
{
   int hdl;
   char LocalNode[100];
   BOOL bIsLocal=FALSE;
   x_strcpy(LocalNode,ResourceString((UINT)IDS_I_LOCALNODE));
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
					if (lpsvr->DBEhdl>=0) {
                        DBEReplReplReleaseEntry(lpsvr->DBEhdl);
						lpsvr->DBEhdl     =DBEHDL_ERROR;
						lpsvr->startstatus=REPSVR_UNKNOWNSTATUS;
                        b2Refresh=TRUE;
					}
				}
			}
		 }
		 if (b2Refresh) 
			 UpdateMonDisplay(hdl, 0,NULL,OT_MON_ALL);
      }
   }
   return RES_SUCCESS;
}
