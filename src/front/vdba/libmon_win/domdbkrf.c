/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : domdbkrf.c
**    provide information whether monitor data are to be refreshed 
**
** History
**
**	22-Nov-2001 (noifr01)
**      extracted and adapted from the winmain\domdbkrf.c source
**  24-Mar-2004 (shaha03)
**		(sir 112040) Added the Multi-thread ability to the LibMon directory. LIBMON_NeedBkRefresh() is the scope within this file
********************************************************************/


#include "libmon.h"
#include "domdloca.h"
#include "error.h"
#include "lbmnmtx.h"

static int  DOMNodeRefresh (
						time_t refreshtime,
						int hnodestruct,
						int iobjecttype,
						void * pstartoflist,
						int level,
						LPUCHAR * parentstrings,
						void * pdispwind,
						BOOL bParentRefreshed,
						BOOL *pbRefreshAll,
						BOOL *pbRefreshMon,
						BOOL bShouldTestOnly,
						BOOL * pbAnimRefreshRequired,
						BOOL bFromAnimation,
						HWND hwndAnimation)

{
   int   i, ires;
   void ** pptemp;
   struct ServConnectData * pdata1 = virtnode[hnodestruct].pdata;
   LPDOMREFRESHPARAMS pRefreshParams;
   UCHAR Parents[MAXPLEVEL][MAXOBJECTNAME];
   LPUCHAR Parents4Call[MAXPLEVEL];
   UCHAR bufObjectWithOwner[MAXOBJECTNAME];
   BOOL bwithsystem;
   BOOL bLocalParentRefreshed;

   bShouldTestOnly = TRUE;

   for (i=0;i<level;i++) {
      fstrncpy(Parents[i], parentstrings[i], MAXOBJECTNAME);
   }
   for (i=0;i<MAXPLEVEL;i++) {
      Parents4Call[i] = Parents[i];
   }
   
   ires = RES_SUCCESS;


   switch (iobjecttype) {

      case OT_VIRTNODE  :
         {
            int itype[]={OT_DATABASE,
                         OT_MON_SERVER,
                         OT_MON_LOCKLIST,
                         OTLL_MON_RESOURCE,
                         OT_MON_LOGPROCESS,    
                         OT_MON_LOGDATABASE,   
                         OT_MON_TRANSACTION};
            if (!pdata1)
               return RES_SUCCESS;
            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     pRefreshParams = &(pdata1->DBRefreshParms);
                     pptemp         = (void **) &(pdata1->lpDBData);
                     bwithsystem    =  pdata1->HasSystemDB;
                     break;
                  case OT_MON_SERVER        :
                     pRefreshParams = &(pdata1->ServerRefreshParms);
                     pptemp         = (void **) &(pdata1->lpServerData);
                     bwithsystem    =  pdata1->HasSystemServers;
                     break;
                  case OT_MON_LOCKLIST      :
                     pRefreshParams = &(pdata1->LockListRefreshParms);
                     pptemp         = (void **) &(pdata1->lpLockListData);
                     bwithsystem    =  pdata1->HasSystemLockLists;
                     break;
                  case OTLL_MON_RESOURCE      :
                     pRefreshParams = &(pdata1->ResourceRefreshParms);
                     pptemp         = (void **) &(pdata1->lpResourceData);
                     bwithsystem    =  pdata1->HasSystemResources;
                     break;
                  case OT_MON_LOGPROCESS    :
                     pRefreshParams = &(pdata1->LogProcessesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpLogProcessData);
                     bwithsystem    =  pdata1->HasSystemLogProcesses;
                     break;
                  case OT_MON_LOGDATABASE   :
                     pRefreshParams = &(pdata1->LogDBRefreshParms);
                     pptemp         = (void **) &(pdata1->lpLogDBData);
                     bwithsystem    =  pdata1->HasSystemLogDB;
                     break;
                  case OT_MON_TRANSACTION:
                     pRefreshParams = &(pdata1->LogTransactRefreshParms);
                     pptemp         = (void **) &(pdata1->lpLogTransactData);
                     bwithsystem    =  pdata1->HasSystemLogTransact;
                     break;
                  default:
                     return RES_ERR;
               }
               bLocalParentRefreshed=FALSE;
               if (*pptemp) {
                  if (DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
					if (bShouldTestOnly) {
						*pbAnimRefreshRequired = TRUE;
						return RES_SUCCESS;
					}
                  }
               }
               DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                               level,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
							   bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
            }
         }
         break;
      case OT_DATABASE  :
         {
            struct DBData * pDBData = (struct DBData * )pstartoflist;
            int itype[]={OT_TABLE,
						 OT_MON_REPLIC_SERVER};
            
            while (pDBData) {
               fstrncpy(Parents[0],pDBData->DBName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_TABLE:
                         pRefreshParams = &(pDBData->TablesRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpTableData);
                         bwithsystem    =  pDBData->HasSystemTables;
                         break;
					  case OT_MON_REPLIC_SERVER:
                         pRefreshParams =&(pDBData->MonReplServerRefreshParms);
                         pptemp = (void **) &(pDBData -> lpMonReplServerData);
                         bwithsystem    =  pDBData->HasSystemMonReplServers;
                         break;
                      default:
                         return RES_ERR;
                  }
                  bLocalParentRefreshed=FALSE;
                  if (*pptemp) {
                     if (DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
						if (bShouldTestOnly) {
							*pbAnimRefreshRequired = TRUE;
							return RES_SUCCESS;
						}
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pDBData=pDBData->pnext;
            }
         }
         break;
      case OT_MON_SESSION       :
      case OT_MON_LOCK          :
      case OT_MON_RES_LOCK      :
      case OT_MON_LOGPROCESS    :
      case OT_MON_LOGDATABASE   :
      case OT_MON_TRANSACTION   :
      case OT_MON_REPLIC_SERVER :
      case OTLL_MON_RESOURCE    :
      case OT_MON_ICE_CONNECTED_USER:
      case OT_MON_ICE_ACTIVE_USER   :
      case OT_MON_ICE_TRANSACTION   :
      case OT_MON_ICE_CURSOR        :
      case OT_MON_ICE_FILEINFO      :
      case OT_MON_ICE_DB_CONNECTION :
      case OT_ICE_ROLE:
      case OT_ICE_DBUSER:
      case OT_ICE_DBCONNECTION:
      case OT_ICE_SERVER_APPLICATION:
      case OT_ICE_SERVER_LOCATION:
      case OT_ICE_SERVER_VARIABLE:
      case OT_ICE_BUNIT_SEC_ROLE:
      case OT_ICE_BUNIT_SEC_USER:
      case OT_ICE_BUNIT_LOCATION:
      case OT_ICE_WEBUSER_ROLE:
      case OT_ICE_WEBUSER_CONNECTION:
      case OT_ICE_PROFILE_ROLE:
      case OT_ICE_PROFILE_CONNECTION:
      case OT_ICE_BUNIT_FACET_ROLE  :
      case OT_ICE_BUNIT_FACET_USER  :
      case OT_ICE_BUNIT_PAGE_ROLE   :
      case OT_ICE_BUNIT_PAGE_USER   :
         return RES_SUCCESS; // no children
         break;
      case OT_USER  :
         return RES_SUCCESS; // no children
         break;
      case OT_TABLE     :
               {
            struct TableData * pTableData = (struct TableData * )pstartoflist;
            BOOL bUsed;
            int itype[]={OT_COLUMN};

            while (pTableData) {
               fstrncpy(Parents[1],
                        StringWithOwner(pTableData->TableName,
                                        pTableData->TableOwner,
                                        bufObjectWithOwner),
                        MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   bUsed  =  TRUE;
                   switch (itype[i]){
                      case OT_COLUMN:
                         pRefreshParams=&(pTableData->ColumnsRefreshParms);
                         pptemp         = (void **) &(pTableData->lpColumnData);
                         bwithsystem    =  pTableData->HasSystemColumns;
                         break;
                      default:
                         return RES_ERR;
                  }
                  bLocalParentRefreshed=FALSE;
                  if (*pptemp) {
                     if (bUsed && DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
						if (bShouldTestOnly) {
							*pbAnimRefreshRequired = TRUE;
							return RES_SUCCESS;
						}
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pTableData=pTableData->pnext;
            }
         }
         break;
      case OT_MON_SERVER :
         {
            struct ServerData * pServerdata = (struct ServerData * )pstartoflist;
            int itype[]={OT_MON_SESSION,
                         OT_MON_ICE_CONNECTED_USER,
                         OT_MON_ICE_ACTIVE_USER,
                         OT_MON_ICE_TRANSACTION,
                         OT_MON_ICE_CURSOR,
                         OT_MON_ICE_FILEINFO,
                         OT_MON_ICE_DB_CONNECTION};

            while (pServerdata) {
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_MON_SESSION:
                        pRefreshParams=&(pServerdata->ServerSessionsRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpServerSessionData);
                        bwithsystem    =  pServerdata->HasSystemServerSessions;
                        break;
                     case OT_MON_ICE_CONNECTED_USER:
                        pRefreshParams=&(pServerdata->IceConnUsersRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceConnUsersData);
                        bwithsystem    =  pServerdata->HasSystemIceConnUsers;
                        break;
                     case OT_MON_ICE_ACTIVE_USER:
                        pRefreshParams=&(pServerdata->IceActiveUsersRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceActiveUsersData);
                        bwithsystem    =  pServerdata->HasSystemIceActiveUsers;
                        break;
                     case OT_MON_ICE_TRANSACTION:
                        pRefreshParams=&(pServerdata->IceTransactionsRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceTransactionsData);
                        bwithsystem    =  pServerdata->HasSystemIceTransactions;
                        break;
                     case OT_MON_ICE_CURSOR:
                        pRefreshParams=&(pServerdata->IceCursorsRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceCursorsData);
                        bwithsystem    =  pServerdata->HasSystemIceCursors;
                        break;
                     case OT_MON_ICE_FILEINFO:
                        pRefreshParams=&(pServerdata->IceCacheRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceCacheInfoData);
                        bwithsystem    =  pServerdata->HasSystemIceCacheInfo;
                        break;
                     case OT_MON_ICE_DB_CONNECTION:
                        pRefreshParams=&(pServerdata->IceDbConnectRefreshParms);
                        pptemp        = (void **)&(pServerdata->lpIceDbConnectData);
                        bwithsystem    =  pServerdata->HasSystemIceDbConnectInfo;
                        break;
                     default:
                        return RES_ERR;
                  }
                  bLocalParentRefreshed=FALSE;
                  if (*pptemp) {
                     if (DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
						if (bShouldTestOnly) {
							*pbAnimRefreshRequired = TRUE;
							return RES_SUCCESS;
						}
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  0,(LPUCHAR *)&(pServerdata->ServerDta), pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pServerdata=pServerdata->pnext;
            }
         }
         break;
      case OT_MON_LOCKLIST :
         {
            struct LockListData * pLockListdata = (struct LockListData * )pstartoflist;
            int itype[]={OT_MON_LOCK};

            while (pLockListdata) {
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_MON_LOCK:
                        pRefreshParams=&(pLockListdata->LocksRefreshParms);
                        pptemp        = (void **)&(pLockListdata->lpLockData);
                        bwithsystem    =  pLockListdata->HasSystemLocks;
                        break;
                     default:
                        return RES_ERR;
                  }
                  bLocalParentRefreshed=FALSE;
                  if (*pptemp) {
                     if (DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
						if (bShouldTestOnly) {
							*pbAnimRefreshRequired = TRUE;
							return RES_SUCCESS;
						}
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  0,(LPUCHAR *)&(pLockListdata->LockListDta), pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pLockListdata=pLockListdata->pnext;
            }
         }
         break;

      case OT_VIEW      :
         {
            struct ViewData * pViewData = (struct ViewData * )pstartoflist;
            int itype[]={OT_VIEWTABLE,
                         OT_VIEWCOLUMN };

            while (pViewData) {
               fstrncpy(Parents[1],
                        StringWithOwner(pViewData->ViewName,
                                        pViewData->ViewOwner,
                                        bufObjectWithOwner),
                        MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_VIEWTABLE:
                        pRefreshParams=&(pViewData->ViewTablesRefreshParms);
                        pptemp = (void **) &(pViewData->lpViewTableData);
                        bwithsystem    = pViewData->HasSystemViewTables;
                        break;
                     case OT_VIEWCOLUMN:
                        pRefreshParams=&(pViewData->ColumnsRefreshParms);
                        pptemp = (void **) &(pViewData->lpColumnData);
                        bwithsystem   =  pViewData->HasSystemColumns;
                        break;
                     default:
                        return RES_ERR;
                  }
                  bLocalParentRefreshed=FALSE;
                  if (*pptemp) {
                     if (DOMIsTimeElapsed(refreshtime,pRefreshParams, bParentRefreshed)) {
						if (bShouldTestOnly) {
							*pbAnimRefreshRequired = TRUE;
							return RES_SUCCESS;
						}
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pViewData=pViewData->pnext;
            }
         }
         break;
      case OT_COLUMN           :
          return RES_SUCCESS;
          break;
      default:
         {
            return myerror(ERR_OBJECTNOEXISTS);
         }
         break;
   }
   return ires;
}
extern LBMNMUTEX lbmn_mtx_Info[MAXNODES];
BOOL LIBMON_NeedBkRefresh (int hnodestruct)
{
	BOOL bRefreshNeeded = FALSE;

	if(!CreateLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Creating a Mutex: Will ignore if the mutex is already created */
		return FALSE;
    if(!OwnLBMNMutex(&lbmn_mtx_Info[hnodestruct],INFINITE)) /*Owning the Mutex */
		return RES_ERR;

	DOMNodeRefresh (ESL_time(), hnodestruct, OT_VIRTNODE, NULL,
                    0,(void *) NULL, (void *) NULL, FALSE, FALSE,
                    FALSE,TRUE, &bRefreshNeeded, FALSE, FALSE);
	if(!UnownLBMNMutex(&lbmn_mtx_Info[hnodestruct])) /*Releasing the mutex before return*/
		return FALSE;
	return bRefreshNeeded;

}

