/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : domdbkrf.c
**    Background refresh
**
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   10-Dec-2001 (noifr01)
**     (sir 99596) removal of obsolete code and resources
**   25-Mar-2003 (noifr01)
**     (sir 107523) management of sequences
********************************************************************/

#include "dba.h"
#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"
#include "domdisp.h"
#include "msghandl.h"
#include "dll.h"
#include "extccall.h"
#include "resource.h"
#include "dlgres.h"


// afx resource define used (idle status bar text)
#ifndef MAINWIN
#include "afxres.h"
#endif

// from mainfrm.cpp
extern void MainFrmUpdateStatusBarPane0(char *buf);

static void RefreshObjects(hnodestruct,iobjecttype,level,parentstrings,
                  bWithSystem,pdispwnd)
int     hnodestruct;
int     iobjecttype;
int     level;
LPUCHAR *parentstrings;
BOOL    bWithSystem;
DISPWND * pdispwnd;

{
   char buf[100];
   if (RefreshMessageOption==REFRESH_WINDOW_POPUP) {
//      if (!*pdispwnd) {
//         *pdispwnd = ESL_OpenRefreshBox();
//         if (!(*pdispwnd)) {
//            myerror(ERR_REFRESH_WIN);
//            return;
//         }
//      }
      if(iobjecttype==OT_NODE)
         x_strcpy(buf,ResourceString(IDS_I_REFRESH_IN_PROGRESS));//"Refresh In Progress"
      else 
         wsprintf(
           buf,
           ResourceString ((UINT)IDS_I_REFRESH_INPROGRESS),    
           GetVirtNodeName(hnodestruct));
  //    ESL_SetRefreshBoxTitle(*pdispwnd,buf);
   }
   if (iobjecttype==OT_VIRTNODE) {
     //
     // Synchronize among objects: Refresh whole node %s
     //
     wsprintf(
           buf,
           ResourceString ((UINT)IDS_I_REFRESH_WHOLE_NODE),
           GetVirtNodeName(hnodestruct));
   }
   else {
      //
      // Vut Begin
      //
      switch (level)
      {
           case 0:
               fstrncpy(buf, ObjectTypeString(iobjecttype,TRUE), 50);
               break;
           case 1:
               wsprintf (
                   buf,
                   ResourceString ((UINT)IDS_I_REFRESH_1), // %s of [%s]
                   ObjectTypeString(iobjecttype,TRUE),
                   parentstrings [0]);
               break;
           case 2:
               wsprintf (
                   buf,
                   ResourceString ((UINT)IDS_I_REFRESH_2), // %s of [%s][%s]
                   ObjectTypeString(iobjecttype,TRUE),
                   parentstrings [0],
                   parentstrings [1]);
               break;
           case 3:
               wsprintf (
                   buf,
                   ResourceString ((UINT)IDS_I_REFRESH_3), // %s of [%s][%s][%s]
                   ObjectTypeString(iobjecttype,TRUE),
                   parentstrings [0],
                   parentstrings [1],
                   parentstrings [2]);
               break;
      }
      //
      // Vut End
   }
   switch (RefreshMessageOption) {
      case REFRESH_WINDOW_POPUP :
			if(iobjecttype!=OT_NODE) {
				x_strcat(buf,ResourceString(IDS_ON));
				x_strcat(buf,GetVirtNodeName(hnodestruct));
			}
			NotifyAnimationDlgTxt(*pdispwnd,buf);
         //ESL_SetRefreshBoxText (*pdispwnd,buf);
         break;
      case REFRESH_STATUSBAR    :
         MainFrmUpdateStatusBarPane0(buf);
         break;
      case REFRESH_NONE         :
         break;
   }
   switch (iobjecttype) {
     case OT_MON_ALL:
       UpdateDOMDataDetail(hnodestruct,  /* node handle  */
                           OT_MON_ALL,   /* object type  */
                           0,            /* level        */
                           (char**)0,    /* parentstrings*/
                           FALSE,        /* bWithSystem  */
                           TRUE,         /* bWithChildren*/
                           TRUE,         /* bOnlyIfExist */
                           FALSE,        /* bUnique      */
                           FALSE);       /* bWithDisplay */
       UpdateMonDisplay(hnodestruct, 0,NULL,iobjecttype);
       break;
     case OT_NODE:
       UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
       UpdateNodeDisplay();
       break;
     case OT_VIRTNODE:
         UpdateDOMDataDetail(hnodestruct,  /* node handle  */
                             OT_VIRTNODE,  /* object type  */
                             0,            /* level        */
                             (char**)0,    /* parentstrings*/
                             FALSE,        /* bWithSystem  */
                             TRUE,         /* bWithChildren*/
                             TRUE,         /* bOnlyIfExist */
                             FALSE,        /* bUnique      */
                             FALSE);       /* bWithDisplay */

         DOMUpdateDisplayData(hnodestruct, /* hnodestruct           */
                              OT_VIRTNODE, /* iobjecttype           */
                              0,           /* level                 */
                              (char**)0,   /* parentstrings         */
                              FALSE,       /* no more in use        */
                              ACT_BKTASK,  /* all must be refreshed */
                              0L,          /* no item id            */
                              0);          /* all doms on the node  */
         UpdateDBEDisplay(hnodestruct,NULL);
		 break;
      default:
         UpdateDOMData(hnodestruct,iobjecttype,level,parentstrings,
                     bWithSystem,FALSE);
         DOMDisableDisplay(hnodestruct, 0);
         DOMUpdateDisplayData (hnodestruct, iobjecttype,level,parentstrings,
                              FALSE,ACT_BKTASK,0L,0);
         if (iobjecttype==OT_DBEVENT)
            UpdateDBEDisplay(hnodestruct,parentstrings[0]);
         DOMEnableDisplay(hnodestruct, 0);
   }

   switch (RefreshMessageOption) {
      case REFRESH_WINDOW_POPUP :
		  NotifyAnimationDlgTxt(*pdispwnd,"");
//         ESL_SetRefreshBoxText (*pdispwnd,"");
         break;
      case REFRESH_STATUSBAR    :
         // bring back default text in pane 0
         buf[0] = '\0';
#ifdef MAINWIN
         x_strcpy(buf, "ready");
#else
         LoadString(hResource, AFX_IDS_IDLEMESSAGE, buf, sizeof(buf));
#endif  //  MAINWIN
         MainFrmUpdateStatusBarPane0(buf);
         break;
      case REFRESH_NONE         :
         break;
   }
}

static int  DOMNodeRefresh (refreshtime, hnodestruct, iobjecttype, pstartoflist,
                    level,parentstrings, pdispwind, bParentRefreshed, pbRefreshAll,
                    pbRefreshMon,bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation)
time_t refreshtime;
int hnodestruct;
int iobjecttype;
void * pstartoflist;
int level;
LPUCHAR * parentstrings;
DISPWND * pdispwind;
BOOL bParentRefreshed;
BOOL *pbRefreshAll;
BOOL *pbRefreshMon;
BOOL bShouldTestOnly;
BOOL * pbAnimRefreshRequired;
BOOL bFromAnimation;
HWND hwndAnimation;

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
   if (HasLoopInterruptBeenRequested())
		return RES_ERR;

   if (bShouldTestOnly && *pbAnimRefreshRequired)
	   return RES_SUCCESS; // reentrant function, should exit if "test only", as soon as we know the animation is required

   for (i=0;i<level;i++) {
      fstrncpy(Parents[i], parentstrings[i], MAXOBJECTNAME);
   }
   for (i=0;i<MAXPLEVEL;i++) {
      Parents4Call[i] = Parents[i];
   }
   
   ires = RES_SUCCESS;

   if (*pbRefreshAll) // global refresh will be done externally
      return ires;

   switch (iobjecttype) {

      case OT_VIRTNODE  :
         {
            int itype[]={OT_DATABASE,
                         OT_PROFILE,
                         OT_USER,
                         OT_GROUP,
                         OT_ROLE,
                         OT_LOCATION,
                         OTLL_DBSECALARM,
                         OTLL_DBGRANTEE,
                         OT_MON_SERVER,
                         OT_MON_LOCKLIST,
                         OTLL_MON_RESOURCE,
                         OT_MON_LOGPROCESS,    
                         OT_MON_LOGDATABASE,   
                         OT_MON_TRANSACTION,
                         OT_ICE_ROLE,
                         OT_ICE_DBUSER,
                         OT_ICE_DBCONNECTION,
                         OT_ICE_WEBUSER,
                         OT_ICE_PROFILE,
                         OT_ICE_BUNIT,
                         OT_ICE_SERVER_APPLICATION,
                         OT_ICE_SERVER_LOCATION,
                         OT_ICE_SERVER_VARIABLE     };
            if (!pdata1)
               return RES_SUCCESS;
            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     pRefreshParams = &(pdata1->DBRefreshParms);
                     pptemp         = (void **) &(pdata1->lpDBData);
                     bwithsystem    =  pdata1->HasSystemDB;
                     break;
                  case OT_PROFILE:
                     pRefreshParams = &(pdata1->ProfilesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpProfilesData);
                     bwithsystem    =  pdata1->HasSystemProfiles;
                     break;
                  case OT_USER:
                     pRefreshParams = &(pdata1->UsersRefreshParms);
                     pptemp         = (void **) &(pdata1->lpUsersData);
                     bwithsystem    =  pdata1->HasSystemUsers;
                     break;
                  case OT_GROUP:
                     pRefreshParams = &(pdata1->GroupsRefreshParms);
                     pptemp         = (void **) &(pdata1->lpGroupsData);
                     bwithsystem    =  pdata1->HasSystemGroups;
                     break;
                  case OT_ROLE:
                     pRefreshParams = &(pdata1->RolesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpRolesData);
                     bwithsystem    =  pdata1->HasSystemRoles;
                     break;
                  case OT_LOCATION:
                     pRefreshParams = &(pdata1->LocationsRefreshParms);
                     pptemp         = (void **) &(pdata1->lpLocationsData);
                     bwithsystem    =  pdata1->HasSystemLocations;
                     break;
                  case OTLL_DBSECALARM:
                     pRefreshParams = &(pdata1->RawDBSecuritiesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpRawDBSecurityData);
                     bwithsystem    =  pdata1->HasSystemRawDBSecurities;
                     break;
                  case OTLL_DBGRANTEE:
                     pRefreshParams = &(pdata1->RawDBGranteesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpRawDBGranteeData);
                     bwithsystem    =  pdata1->HasSystemRawDBGrantees;
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
                  case OT_ICE_ROLE:
                     pRefreshParams = &(pdata1->IceRolesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceRolesData);
                     bwithsystem    =  pdata1->HasSystemIceRoles;
                     break;
                   case OT_ICE_DBUSER:
                     pRefreshParams = &(pdata1->IceDbUsersRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceDbUsersData);
                     bwithsystem    =  pdata1->HasSystemIceDbUsers;
                     break;
                   case OT_ICE_DBCONNECTION:
                     pRefreshParams = &(pdata1->IceDbConnectRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceDbConnectionsData);
                     bwithsystem    =  pdata1->HasSystemIceDbConnections;
                     break;
                   case OT_ICE_WEBUSER:
                     pRefreshParams = &(pdata1->IceWebUsersRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceWebUsersData);
                     bwithsystem    =  pdata1->HasSystemIceWebusers;
                     break;
                   case OT_ICE_PROFILE:
                     pRefreshParams = &(pdata1->IceProfilesRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceProfilesData);
                     bwithsystem    =  pdata1->HasSystemIceProfiles;
                     break;
                   case OT_ICE_BUNIT:
                     pRefreshParams = &(pdata1->IceBusinessUnitsRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceBusinessUnitsData);
                     bwithsystem    =  pdata1->HasSystemIceBusinessUnits;
                     break;
                   case OT_ICE_SERVER_APPLICATION:
                     pRefreshParams = &(pdata1->IceApplicationRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceApplicationsData);
                     bwithsystem    =  pdata1->HasSystemIceApplications;
                     break;
                   case OT_ICE_SERVER_LOCATION:
                     pRefreshParams = &(pdata1->IceLocationRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceLocationsData);
                     bwithsystem    =  pdata1->HasSystemIceLocations;
                     break;
                   case OT_ICE_SERVER_VARIABLE:
                     pRefreshParams = &(pdata1->IceVarRefreshParms);
                     pptemp         = (void **) &(pdata1->lpIceVarsData);
                     bwithsystem    =  pdata1->HasSystemIceVars;
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
                     if (pRefreshParams->iobjecttype==OT_MON_ALL){
                        *pbRefreshMon=TRUE;
                        continue;
                     }
                     if (RefreshSyncAmongObject) {
                        *pbRefreshAll=TRUE;
                        return ires;
                     }
                     RefreshObjects(hnodestruct,itype[i],0,Parents4Call,
                                        bwithsystem, pdispwind);
                     // error already handled in called function
                     bLocalParentRefreshed=TRUE;
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
                         OTLL_OIDTDBGRANTEE,
                         OTLL_SECURITYALARM,
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
                      case OT_VIEW:
                         pRefreshParams = &(pDBData->ViewsRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpViewData);
                         bwithsystem    =  pDBData->HasSystemViews;
                         break;
                      case OT_PROCEDURE:
                         pRefreshParams = &(pDBData->ProceduresRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpProcedureData);
                         bwithsystem    =  pDBData->HasSystemProcedures;
                         break;
                      case OT_SEQUENCE:
                         pRefreshParams = &(pDBData->SequencesRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpSequenceData);
                         bwithsystem    =  pDBData->HasSystemSequences;
                         break;
                      case OT_SCHEMAUSER:
                         pRefreshParams = &(pDBData->SchemasRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpSchemaUserData);
                         bwithsystem    =  pDBData->HasSystemSchemas;
                         break;
                      case OT_SYNONYM:
                         pRefreshParams = &(pDBData->SynonymsRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpSynonymData);
                         bwithsystem    =  pDBData->HasSystemSynonyms;
                         break;
                      case OT_DBEVENT:
                         pRefreshParams = &(pDBData->DBEventsRefreshParms);
                         pptemp         = (void **) &(pDBData -> lpDBEventData);
                         bwithsystem    =  pDBData->HasSystemDBEvents;
                         break;
                      case OT_REPLIC_CONNECTION:
                         pRefreshParams = &(pDBData->ReplicConnectionsRefreshParms);
                         pptemp         =(void **) &(pDBData->lpReplicConnectionData);
                         bwithsystem    =  pDBData->HasSystemReplicConnections;
                         break;
                      case OT_REPLIC_REGTABLE:
                         pRefreshParams=&(pDBData->ReplicRegTablesRefreshParms);
                         pptemp        = (void **)&(pDBData ->lpReplicRegTableData);
                         bwithsystem    =  pDBData->HasSystemReplicRegTables;
                         break;
                      case OT_REPLIC_MAILUSER:
                         pRefreshParams=&(pDBData->ReplicMailErrsRefreshParms);
                         pptemp= (void **) &(pDBData -> lpReplicMailErrData);
                         bwithsystem    =  pDBData->HasSystemReplicMailErrs;
                         break;
                      case OTLL_REPLIC_CDDS:
                         pRefreshParams=&(pDBData->RawReplicCDDSRefreshParms);
                         pptemp =(void **)&(pDBData -> lpRawReplicCDDSData);
                         bwithsystem    =  pDBData->HasSystemRawReplicCDDS;
                         break;
                      case OTLL_GRANTEE:
                         pRefreshParams = &(pDBData->RawGranteesRefreshParms);
                         pptemp = (void **) &(pDBData -> lpRawGranteeData) ;
                         bwithsystem    =  pDBData->HasSystemRawGrantees;
                         break;
                      case OTLL_OIDTDBGRANTEE:
                           continue;
                         break;
                      case OTLL_SECURITYALARM:
                         pRefreshParams =&(pDBData->RawSecuritiesRefreshParms);
                         pptemp = (void **) &(pDBData -> lpRawSecurityData);
                         bwithsystem    =  pDBData->HasSystemRawSecurities;
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
						if (pRefreshParams->iobjecttype==OT_MON_ALL){
							*pbRefreshMon=TRUE;
							continue;
						}
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
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
      case OT_ICE_BUNIT_FACET:
         {
            struct IceObjectWithRolesAndUsersData * pFacetData = (struct IceObjectWithRolesAndUsersData * )pstartoflist;
            int itype[]={OT_ICE_BUNIT_FACET_ROLE,
                         OT_ICE_BUNIT_FACET_USER};

            while (pFacetData) {
               fstrncpy(Parents[1],pFacetData->ObjectName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_FACET_ROLE:
                         pRefreshParams =&(pFacetData->ObjectRolesRefreshParms);
                         pptemp         = (void **) &(pFacetData->lpObjectRolesData);
                         bwithsystem    =  pFacetData->HasSystemObjectRoles;
                         break;
                      case OT_ICE_BUNIT_FACET_USER:
                         pRefreshParams=&(pFacetData->ObjectUsersRefreshParms);
                         pptemp         = (void **) &(pFacetData->lpObjectUsersData);
                         bwithsystem    =  pFacetData->HasSystemObjectUsers;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],2,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pFacetData=pFacetData->pnext;
            }
         }
         break;
      case OT_ICE_BUNIT_PAGE:
         {
            struct IceObjectWithRolesAndUsersData * pPageData = (struct IceObjectWithRolesAndUsersData * )pstartoflist;
            int itype[]={OT_ICE_BUNIT_PAGE_ROLE,
                         OT_ICE_BUNIT_PAGE_USER};

            while (pPageData) {
               fstrncpy(Parents[1],pPageData->ObjectName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_PAGE_ROLE:
                         pRefreshParams =&(pPageData->ObjectRolesRefreshParms);
                         pptemp         = (void **) &(pPageData->lpObjectRolesData);
                         bwithsystem    =  pPageData->HasSystemObjectRoles;
                         break;
                      case OT_ICE_BUNIT_PAGE_USER:
                         pRefreshParams=&(pPageData->ObjectUsersRefreshParms);
                         pptemp         = (void **) &(pPageData->lpObjectUsersData);
                         bwithsystem    =  pPageData->HasSystemObjectUsers;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],2,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pPageData=pPageData->pnext;
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


      case OT_PROFILE:
         return RES_SUCCESS; // no children
         break;
      case OT_USER  :
         return RES_SUCCESS; // no children
         break;
      case OT_ICE_WEBUSER:
         {
            struct IceObjectWithRolesAndDBData * pWebUserData = (struct IceObjectWithRolesAndDBData * )pstartoflist;
            int itype[]={OT_ICE_WEBUSER_ROLE,
                         OT_ICE_WEBUSER_CONNECTION};

            while (pWebUserData) {
               fstrncpy(Parents[0],pWebUserData->ObjectName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_WEBUSER_ROLE:
                         pRefreshParams =&(pWebUserData->ObjectRolesRefreshParms);
                         pptemp         = (void **) &(pWebUserData->lpObjectRolesData);
                         bwithsystem    =  pWebUserData->HasSystemObjectRoles;
                         break;
                      case OT_ICE_WEBUSER_CONNECTION:
                         pRefreshParams=&(pWebUserData->ObjectDbRefreshParms);
                         pptemp         = (void **) &(pWebUserData->lpObjectDbData);
                         bwithsystem    =  pWebUserData->HasSystemObjectDbs;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pWebUserData=pWebUserData->pnext;
            }
         }
         break;
      case OT_ICE_PROFILE:
         {
            struct IceObjectWithRolesAndDBData * pIceProfileData = (struct IceObjectWithRolesAndDBData * )pstartoflist;
            int itype[]={OT_ICE_PROFILE_ROLE,
                         OT_ICE_PROFILE_CONNECTION};

            while (pIceProfileData) {
               fstrncpy(Parents[0],pIceProfileData->ObjectName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_PROFILE_ROLE:
                         pRefreshParams =&(pIceProfileData->ObjectRolesRefreshParms);
                         pptemp         = (void **) &(pIceProfileData->lpObjectRolesData);
                         bwithsystem    =  pIceProfileData->HasSystemObjectRoles;
                         break;
                      case OT_ICE_PROFILE_CONNECTION:
                         pRefreshParams=&(pIceProfileData->ObjectDbRefreshParms);
                         pptemp         = (void **) &(pIceProfileData->lpObjectDbData);
                         bwithsystem    =  pIceProfileData->HasSystemObjectDbs;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pIceProfileData=pIceProfileData->pnext;
            }
         }
         break;
      case OT_ICE_BUNIT:
     {
            struct IceBusinessUnitsData * pBusUnitData = (struct IceBusinessUnitsData * )pstartoflist;
            int itype[]={OT_ICE_BUNIT_SEC_ROLE,
                         OT_ICE_BUNIT_SEC_USER,
                         OT_ICE_BUNIT_FACET,
                         OT_ICE_BUNIT_PAGE,
                         OT_ICE_BUNIT_LOCATION};

            while (pBusUnitData) {
               fstrncpy(Parents[0],pBusUnitData->BusinessunitName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_SEC_ROLE:
                         pRefreshParams =&(pBusUnitData->ObjectRolesRefreshParms);
                         pptemp         = (void **) &(pBusUnitData->lpObjectRolesData);
                         bwithsystem    =  pBusUnitData->HasSystemObjectRoles;
                         break;
                      case OT_ICE_BUNIT_SEC_USER:
                         pRefreshParams=&(pBusUnitData->ObjectUsersRefreshParms);
                         pptemp         = (void **) &(pBusUnitData->lpObjectUsersData);
                         bwithsystem    =  pBusUnitData->HasSystemObjectUsers;
                         break;
                      case OT_ICE_BUNIT_FACET:
                         pRefreshParams =&(pBusUnitData->FacetRefreshParms);
                         pptemp         = (void **) &(pBusUnitData->lpObjectFacetsData);
                         bwithsystem    =  pBusUnitData->HasSystemFacets;
                         break;
                      case OT_ICE_BUNIT_PAGE:
                         pRefreshParams =&(pBusUnitData->PagesRefreshParms);
                         pptemp         = (void **) &(pBusUnitData->lpObjectPagesData);
                         bwithsystem    =  pBusUnitData->HasSystemPages;
                         break;
                      case OT_ICE_BUNIT_LOCATION:
                         pRefreshParams =&(pBusUnitData->ObjectLocationsRefreshParms);
                         pptemp         = (void **) &(pBusUnitData->lpObjectLocationsData);
                         bwithsystem    =  pBusUnitData->HasSystemObjectLocations;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pBusUnitData=pBusUnitData->pnext;
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
                        *pbRefreshMon=TRUE;
                        continue;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  0,&(pServerdata->ServerDta), pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
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
                        *pbRefreshMon=TRUE;
                        continue;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  0,&(pLockListdata->LockListDta), pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pLockListdata=pLockListdata->pnext;
            }
         }
         break;
      case OT_GROUP :
         {
            struct GroupData * pGroupdata = (struct GroupData * )pstartoflist;
            int itype[]={OT_GROUPUSER };

            while (pGroupdata) {
               fstrncpy(Parents[0],pGroupdata->GroupName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_GROUPUSER:
                        pRefreshParams=&(pGroupdata->GroupUsersRefreshParms);
                        pptemp        = (void **)&(pGroupdata->lpGroupUserData);
                        bwithsystem    =  pGroupdata->HasSystemGroupUsers;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pGroupdata=pGroupdata->pnext;
            }
         }
         break;
      case OT_GROUPUSER :
         return RES_SUCCESS; // no children
         break;
      case OT_ROLE      :
         {
            struct RoleData * pRoledata = (struct RoleData * )pstartoflist;
            int itype[]={OT_ROLEGRANT_USER };

            while (pRoledata) {
               fstrncpy(Parents[0],pRoledata->RoleName,MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_ROLEGRANT_USER:
                        pRefreshParams=&(pRoledata->RoleGranteesRefreshParms);
                        pptemp         = (void **)&(pRoledata->lpRoleGranteeData);
                        bwithsystem    =  pRoledata->HasSystemRoleGrantees;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],1,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
                     }
                  }
                  DOMNodeRefresh (refreshtime, hnodestruct, itype[i], *pptemp,
                                  1,Parents4Call, pdispwind, bLocalParentRefreshed, pbRefreshAll, pbRefreshMon,
								  bShouldTestOnly,pbAnimRefreshRequired,bFromAnimation, hwndAnimation);
               }
               pRoledata=pRoledata->pnext;
            }
         }
         break;
      case OT_ROLEGRANT_USER :
      case OT_LOCATION  :
         return RES_SUCCESS; // no children
         break;

      case OT_TABLE     :
         {
            struct TableData * pTableData = (struct TableData * )pstartoflist;
            BOOL bUsed;
            int itype[]={OT_INTEGRITY,
                         OT_COLUMN,
                         OT_RULE,
                         OT_INDEX,
                         OT_TABLELOCATION };

            while (pTableData) {
               fstrncpy(Parents[1],
                        StringWithOwner(pTableData->TableName,
                                        pTableData->TableOwner,
                                        bufObjectWithOwner),
                        MAXOBJECTNAME);
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   bUsed  =  TRUE;
                   switch (itype[i]){
                      case OT_INTEGRITY:
                         pRefreshParams =&(pTableData->IntegritiesRefreshParms);
                         pptemp         = (void **) &(pTableData->lpIntegrityData);
                         bwithsystem    =  pTableData->HasSystemIntegrities;
                         bUsed          =  pTableData->integritiesused;
                         break;
                      case OT_COLUMN:
                         pRefreshParams=&(pTableData->ColumnsRefreshParms);
                         pptemp         = (void **) &(pTableData->lpColumnData);
                         bwithsystem    =  pTableData->HasSystemColumns;
                         break;
                      case OT_RULE:
                         pRefreshParams=&(pTableData->RulesRefreshParms);
                         pptemp         = (void **) &(pTableData->lpRuleData);
                         bwithsystem    =  pTableData->HasSystemRules;
                         break;
                      case OT_INDEX:
                         pRefreshParams=&(pTableData->IndexesRefreshParms);
                         pptemp         = (void **) &(pTableData->lpIndexData);
                         bwithsystem    =  pTableData->HasSystemIndexes;
                         bUsed          =  pTableData->indexesused;
                         break;
                      case OT_TABLELOCATION:
                         pRefreshParams=&(pTableData->TableLocationsRefreshParms);
                         pptemp         =(void **)&(pTableData->lpTableLocationData);
                         bwithsystem    =  pTableData->HasSystemTableLocations;
                         bUsed          =  pTableData->tablelocationsused;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],2,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
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
                        if (RefreshSyncAmongObject) {
                           *pbRefreshAll=TRUE;
                           return ires;
                        }
                        RefreshObjects(hnodestruct,itype[i],2,Parents4Call,
                                      bwithsystem, pdispwind);
                        // error already handled in called function
                        bLocalParentRefreshed=TRUE;
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
      case OT_INTEGRITY        :
      case OT_COLUMN           :
      case OT_VIEWCOLUMN       :
      case OT_RULE             :
      case OT_INDEX            :
      case OT_TABLELOCATION    :
      case OT_PROCEDURE        :
      case OT_SEQUENCE         :
      case OT_SCHEMAUSER       :
      case OT_SYNONYM          :
      case OT_DBEVENT          :
      case OT_REPLIC_CONNECTION:
      case OT_REPLIC_REGTABLE  :
      case OT_REPLIC_MAILUSER  :
      case OTLL_REPLIC_CDDS    :
      case OT_VIEWTABLE        :
      case OTLL_DBGRANTEE      :
      case OTLL_DBSECALARM     :
      case OTLL_GRANTEE        :
      case OTLL_OIDTDBGRANTEE  :
      case OTLL_SECURITYALARM  :
         return RES_SUCCESS; // no children
         break;
      default:
         {
            char toto[40];
            // type of error2 %d
            wsprintf(toto,ResourceString ((UINT)IDS_E_TYPEOF_ERROR2));
            TS_MessageBox( TS_GetFocus(), toto, NULL, MB_OK | MB_TASKMODAL);
            return myerror(ERR_OBJECTNOEXISTS);
         }
         break;
   }
   return ires;
}


int DOMBkRefresh(refreshtime,bFromAnimation,hwndAnimation)
time_t refreshtime;
BOOL bFromAnimation;
HWND hwndAnimation;
{
   int i;
   DISPWND dispwnd= (DISPWND)0;
   BOOL bRefreshAll,bRefreshMon;
   BOOL bShouldTestOnly     = FALSE;
   BOOL bAnimRefreshRequired = FALSE;
   if (!bFromAnimation && RefreshMessageOption==REFRESH_WINDOW_POPUP)
	   bShouldTestOnly = TRUE;
   if (bFromAnimation)
	   dispwnd = hwndAnimation;
   for (i=0;i<imaxnodes;i++)  {
      bRefreshAll = FALSE;
      bRefreshMon = FALSE;
      DOMNodeRefresh (refreshtime,i, OT_VIRTNODE, (void *) NULL,
                      0, (void *) NULL, &dispwnd, FALSE, &bRefreshAll, &bRefreshMon,
					  bShouldTestOnly,&bAnimRefreshRequired,bFromAnimation, hwndAnimation);
	  if (bShouldTestOnly && bAnimRefreshRequired)
		  return Task_BkRefresh(refreshtime); 
      if (bRefreshAll) {
        RefreshObjects(i,OT_VIRTNODE,0,(char**)0,FALSE,&dispwnd);
      }
      if (bRefreshMon)  {
        if (isVirtNodeEntryBusy(i)) {
          if (isMonSpecialState(i)) {
             SetMonNormalState(i);
             UpdateMonDisplay(i,OT_MON_ALL,NULL, OT_MON_ALL);
             continue;
          }
        }
        RefreshObjects(i,OT_MON_ALL,0,(char**)0,FALSE,&dispwnd);
      }
   }

   if (lpMonNodeData &&
       DOMIsTimeElapsed(refreshtime,&(MonNodesRefreshParms),FALSE)) {
		if (bShouldTestOnly) {
			bAnimRefreshRequired = TRUE;
			return Task_BkRefresh(refreshtime); 
		}
     RefreshObjects(0,OT_NODE,0,NULL,TRUE,&dispwnd);
     UpdateNodeDisplay();
   }
   RefreshPropWindows(FALSE,refreshtime);

//   if (dispwnd)
//      ESL_CloseRefreshBox(dispwnd);

   return RES_SUCCESS;
}


void RefreshOnLoad()
{
   BOOL bWithChildren;
   int i1,i2,iloc,iobjtype[20],nbobjtypes,level;
   LPUCHAR Parents4Call[MAXPLEVEL];
   BOOL bMonSMDisplayed;
   BOOL bNodesTobeRefreshed;
   for (iloc=0;iloc<MAXPLEVEL;iloc++) 
      Parents4Call[iloc] = (LPUCHAR) 0;
   bMonSMDisplayed=FALSE;
   bNodesTobeRefreshed = FALSE;
   for (i1=0;i1<imaxnodes;i1++)  {
      BOOL bRefreshOccured  = FALSE;
      BOOL bTablesRefreshed = FALSE;
      BOOL bMonTobeRefreshed= FALSE;
      BOOL bDBETobeRefreshed= FALSE;
      if (!virtnode[i1].pdata)
         continue;
      for (i2 = 0; i2 < MAXREFRESHOBJTYPES; i2++) {
         if (FreqSettings [i2].bOnLoad) {
            iobjtype[0]=ObjToBeRefreshed[i2];
            nbobjtypes=1;
            bRefreshOccured = TRUE;
            bWithChildren   = FALSE;
            switch (iobjtype[0]) {
               case OT_DATABASE  :
               case OT_PROFILE   :
               case OT_USER      :
               case OT_GROUP     :
               case OT_ROLE      :
               case OT_LOCATION  :
               case OT_MON_SERVER        :
               case OT_MON_LOCKLIST      :
               case OTLL_MON_RESOURCE      :
               case OT_MON_LOGPROCESS    :
               case OT_MON_LOGDATABASE   :
               case OT_MON_TRANSACTION:
                  level=0;
                  break;
               case OTLL_DBGRANTEE :
                     level=0;
                  break;
               case OT_DBEVENT   :
                  bDBETobeRefreshed=TRUE;
                  level=1;
                  break;

               case OT_GROUPUSER :
               case OT_VIEW      :
               case OT_SCHEMAUSER:
               case OT_SYNONYM   :
               case OT_PROCEDURE :
               case OT_SEQUENCE  :
               case OTLL_SECURITYALARM :
                  level=1;
                  break;
               case OT_VIEWTABLE :
               case OT_RULE      :
               case OT_COLUMN    :
                  level=2;
                  break;
               case OTLL_GRANTEE :
               case OTLL_GRANT   :
                  iobjtype[0]=OTLL_GRANTEE;
                  level=1;
                  break;
               case OT_REPLIC_CONNECTION : // handle all replication objects
                  iobjtype[0] = OT_REPLIC_CONNECTION;
                  iobjtype[1] = OT_REPLIC_REGTABLE  ;
                  iobjtype[2] = OT_REPLIC_MAILUSER  ;
                  iobjtype[3] = OTLL_REPLIC_CDDS    ;
                  nbobjtypes = 4;
                  level = 1;
                  break;
               case OT_TABLE:
                  bTablesRefreshed = TRUE;
                  bWithChildren=TRUE;
                  level=1;
                  break;
               case OT_INDEX:
               case OT_INTEGRITY:
               case OT_TABLELOCATION:   // isolated for speed reason
                  if (bTablesRefreshed)
                     continue;
                  bTablesRefreshed = TRUE;
                  bWithChildren=TRUE;
                  iobjtype[0]=OT_TABLE;
                  level=1;
                  break;
               case OT_MON_ALL:   // isolated for speed reason
                  bMonTobeRefreshed= TRUE;
                  nbobjtypes = 0;
                  continue;
               case OT_NODE:
                  bNodesTobeRefreshed = TRUE;
                  nbobjtypes = 0;
                  continue;
               case OT_ICE_GENERIC:
                  bWithChildren=TRUE;
                  iobjtype[0] = OT_ICE_ROLE;
                  iobjtype[1] = OT_ICE_DBUSER  ;
                  iobjtype[2] = OT_ICE_DBCONNECTION  ;
                  iobjtype[3] = OT_ICE_WEBUSER    ;
                  iobjtype[4] = OT_ICE_PROFILE    ;
                  iobjtype[5] = OT_ICE_BUNIT    ;
                  iobjtype[6] = OT_ICE_SERVER_APPLICATION    ;
                  iobjtype[7] = OT_ICE_SERVER_LOCATION    ;
                  iobjtype[8] = OT_ICE_SERVER_VARIABLE    ;
                  nbobjtypes = 9;
                  level = 0;
                  break;
               default:
                  myerror(ERR_BKRF);
                  continue;
            }
            for (iloc=0;iloc<nbobjtypes;iloc++) {
               UpdateDOMDataDetail(i1,
                                   iobjtype[iloc],
                                   level,
                                   Parents4Call,
                                   FALSE,           // bWithSystem
                                   bWithChildren,
                                   TRUE,            // bOnlyIfExist
                                   FALSE,           // bUnique
                                   FALSE            // bWithDisplay
                                  );
            }
         }
      }
      if (bMonTobeRefreshed) {
        if (isVirtNodeEntryBusy(i1)) {
           SetMonNormalState(i1);
           UpdateMonDisplay(i1, OT_MON_ALL,NULL, OT_MON_ALL);
        }
      }
      else {
        if (!bMonSMDisplayed) {
          if (MonOpenWindowsCount(i1)) {
             TS_MessageBox(TS_GetFocus(),
				ResourceString(IDS_MSG_ONLOAD_SHOULDREFRESH_MON),
				ResourceString(IDS_TITLE_PERF_MON_WINDOWS),
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
             bMonSMDisplayed = TRUE;
          }
          else {
            if (isVirtNodeEntryBusy(i1))
              SetMonNormalState(i1);
          }
        }
      }
      if (bDBETobeRefreshed)
        UpdateDBEDisplay(i1, NULL);
      if (bRefreshOccured) {
        DOMUpdateDisplayData(i1,          /* hnodestruct           */
                             OT_VIRTNODE, /* iobjecttype           */
                             0,           /* level                 */
                             (char**)0,   /* parentstrings         */
                             FALSE,       /* no more in use        */
                             ACT_BKTASK,  /* all must be refreshed */
                             0L,          /* no item id            */
                             0);          /* all doms on the node  */
      }
   }
   if (bNodesTobeRefreshed) {
     UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
     UpdateNodeDisplay();
   }
   RefreshPropWindows(TRUE,ESL_time()); //Refresh property windows if needed
}

