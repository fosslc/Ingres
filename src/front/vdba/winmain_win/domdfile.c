/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**
**    Source : domdfile.c
**    Save/Load cache
**
**  history :
**   09-Nov-99 (noifr01)
**    (bug 99445) fixed incorrect cast for groupusers sub-list of a group
**   19-Jan-2000 (noifr01)
**    (bug 100063) revoved the DOMTestSaveAndLoad() function that was
**    providing a warning, and isn't used any more (old test function)
**   26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**   10-Dec-2001 (noifr01)
**     (sir 99596) removal of obsolete code and resources
**   25-Mar-2003 (noifr01)
**     (sir 107523) management of sequences
********************************************************************/

#include <stdio.h>
#include "dba.h"
#include "dbafile.h"
#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"
#include "msghandl.h"
#include "dll.h"

int  DOMSaveList ( fident, hnodestruct, iobjecttype, pstartoflist)
FILEIDENT fident;
int hnodestruct;
int iobjecttype;
void * pstartoflist;
{
   int   i, ires;
   BOOL  btemp;
   void * ptemp;
   struct ServConnectData * pdata1 = virtnode[hnodestruct].pdata;
   char buftmp[MAXOBJECTNAME];

   ires = RES_SUCCESS;

   switch (iobjecttype) {

      case OT_VIRTNODE  :
         {
			int users;
            int itype[]={OT_DATABASE,
                         OT_PROFILE,
                         OT_USER,
                         OT_GROUP,
                         OT_ROLE,
                         OT_LOCATION,
                         OTLL_DBSECALARM,
                         OTLL_DBGRANTEE,
                   //     ,               // cache not saved for monitor stuff
                   //      OT_MON_SERVER,
                   //      OT_MON_LOCKLIST,
                   //      OTLL_MON_RESOURCE,
                   //      OT_MON_LOGPROCESS,    
                   //      OT_MON_LOGDATABASE,   
                   //      OT_MON_TRANSACTION
                         OT_ICE_ROLE,
                         OT_ICE_DBUSER,
                         OT_ICE_DBCONNECTION,
                         OT_ICE_WEBUSER,
                         OT_ICE_PROFILE,
                         OT_ICE_BUNIT,
                         OT_ICE_SERVER_APPLICATION,
                         OT_ICE_SERVER_LOCATION,
                         OT_ICE_SERVER_VARIABLE
                         
                        };
         
            btemp  = pdata1? TRUE:FALSE;

            ires = DBAAppend4Save(fident, &btemp, sizeof(btemp));
            if (!btemp || ires!=RES_SUCCESS)
               return ires;

            users=virtnode[hnodestruct].nUsers-DBECacheEntriesUsedByReplMon(hnodestruct);

            ires = DBAAppend4Save(fident, &users,sizeof(users));
            if (!btemp || ires!=RES_SUCCESS)
               return ires;
            
            
			x_strcpy(buftmp,pdata1->ICEUserPassword); // will be restored after the save operation
			x_strcpy(pdata1->ICEUserPassword,""); // don't save the password (will be re-prompted after loading

            ires = DBAAppend4Save(fident, pdata1, sizeof(struct ServConnectData));
			x_strcpy(pdata1->ICEUserPassword,buftmp); // restore the password for current VDBA session
            if (ires!=RES_SUCCESS)
               return ires;
            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     ptemp = (void *) pdata1->lpDBData          ; break;
                  case OT_PROFILE:
                     ptemp = (void *) pdata1->lpProfilesData    ; break;
                  case OT_USER:
                     ptemp = (void *) pdata1->lpUsersData       ; break;
                  case OT_GROUP:
                     ptemp = (void *) pdata1->lpGroupsData      ; break;
                  case OT_ROLE:
                     ptemp = (void *) pdata1->lpRolesData       ; break;
                  case OT_LOCATION:
                     ptemp = (void *) pdata1->lpLocationsData   ; break;
                  case OTLL_DBSECALARM:
                     ptemp = (void *) pdata1->lpRawDBSecurityData; break;
                  case OTLL_DBGRANTEE:
                     ptemp = (void *) pdata1->lpRawDBGranteeData; break;
                  case OT_MON_SERVER        :
                     ptemp = (void *) pdata1->lpServerData     ; break;
                  case OT_MON_LOCKLIST      :
                     ptemp = (void *) pdata1->lpLockListData   ; break;
                  case OTLL_MON_RESOURCE      :
                     ptemp = (void *) pdata1->lpResourceData   ; break;
                  case OT_MON_LOGPROCESS    :
                     ptemp = (void *) pdata1->lpLogProcessData ; break;
                  case OT_MON_LOGDATABASE   :
                     ptemp = (void *) pdata1->lpLogDBData      ; break;
                  case OT_MON_TRANSACTION:
                     ptemp = (void *) pdata1->lpLogTransactData; break;
                  case OT_ICE_ROLE:
                     ptemp = (void *) pdata1->lpIceRolesData ; break;
                  case OT_ICE_DBUSER:
                     ptemp = (void *) pdata1->lpIceDbUsersData ; break;
                  case OT_ICE_DBCONNECTION:
                     ptemp = (void *) pdata1->lpIceDbConnectionsData ; break;
                  case OT_ICE_WEBUSER:
                     ptemp = (void *) pdata1->lpIceWebUsersData ; break;
                  case OT_ICE_PROFILE:
                     ptemp = (void *) pdata1->lpIceProfilesData ; break;
                  case OT_ICE_BUNIT:
                     ptemp = (void *) pdata1->lpIceBusinessUnitsData ; break;
                  case OT_ICE_SERVER_APPLICATION:
                     ptemp = (void *) pdata1->lpIceApplicationsData ; break;
                  case OT_ICE_SERVER_LOCATION:
                     ptemp = (void *) pdata1->lpIceLocationsData ; break;
                  case OT_ICE_SERVER_VARIABLE:
                     ptemp = (void *) pdata1->lpIceVarsData ; break;
                    default:
                     return RES_ERR;
               }
               ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
               if (ires!=RES_SUCCESS)
                  return ires;
            }
         }
         break;
      case OT_ICE_WEBUSER:
      case OT_ICE_PROFILE:
         {
            struct IceObjectWithRolesAndDBData * pdata =
                                      (struct IceObjectWithRolesAndDBData * )pstartoflist;
            int itype[]={OT_ICE_WEBUSER_ROLE,
                         OT_ICE_WEBUSER_CONNECTION };
			if (iobjecttype==OT_ICE_PROFILE) {
				itype[0]=OT_ICE_PROFILE_ROLE;
				itype[1]=OT_ICE_PROFILE_CONNECTION;
			};

            while (pdata) {
               ires = DBAAppend4Save(fident, pdata,
                                             sizeof(struct IceObjectWithRolesAndDBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_WEBUSER_ROLE:
                      case OT_ICE_PROFILE_ROLE:
                         ptemp = (void *) pdata->lpObjectRolesData;break;
                      case OT_ICE_WEBUSER_CONNECTION:
                      case OT_ICE_PROFILE_CONNECTION:
                         ptemp =(void *)  pdata->lpObjectDbData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                   if (ires!=RES_SUCCESS)
                     return ires;
               }
               pdata=pdata->pnext;
            }
         }
         break;
      case OT_ICE_BUNIT:
         {
            struct IceBusinessUnitsData * pdata =
                                      (struct IceBusinessUnitsData * )pstartoflist;
            int itype[]={OT_ICE_BUNIT_SEC_ROLE,
                         OT_ICE_BUNIT_SEC_USER,
                         OT_ICE_BUNIT_FACET,
                         OT_ICE_BUNIT_PAGE,
                         OT_ICE_BUNIT_LOCATION};
            while (pdata) {
               ires = DBAAppend4Save(fident, pdata,
                                             sizeof(struct IceBusinessUnitsData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_SEC_ROLE:
                         ptemp = (void *)pdata->lpObjectRolesData;break;
                      case OT_ICE_BUNIT_SEC_USER:
                         ptemp = (void *)pdata->lpObjectUsersData;break;
                      case OT_ICE_BUNIT_FACET:
                         ptemp = (void *)pdata->lpObjectFacetsData;break;
                      case OT_ICE_BUNIT_PAGE:
                         ptemp = (void *)pdata->lpObjectPagesData;break;
                      case OT_ICE_BUNIT_LOCATION:
                         ptemp = (void *)pdata->lpObjectLocationsData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                   if (ires!=RES_SUCCESS)
                     return ires;
                }
               pdata=pdata->pnext;
             }
         }
         break;
      case OT_ICE_BUNIT_FACET:
      case OT_ICE_BUNIT_PAGE:
         {
            struct IceObjectWithRolesAndUsersData * pdata =
                                      (struct IceObjectWithRolesAndUsersData * )pstartoflist;
            int itype[]={OT_ICE_BUNIT_PAGE_ROLE,
                         OT_ICE_BUNIT_PAGE_USER };
			if (iobjecttype==OT_ICE_BUNIT_FACET) {
				itype[0]=OT_ICE_BUNIT_FACET_ROLE;
                itype[1]=OT_ICE_BUNIT_FACET_USER;
			}

            while (pdata) {
               ires = DBAAppend4Save(fident, pdata,
                                             sizeof(struct IceObjectWithRolesAndUsersData));
                if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_PAGE_ROLE:
                      case OT_ICE_BUNIT_FACET_ROLE:
                         ptemp = (void *)pdata->lpObjectRolesData;break;
                      case OT_ICE_BUNIT_PAGE_USER:
                      case OT_ICE_BUNIT_FACET_USER:
                         ptemp = (void *)pdata->lpObjectUsersData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                   if (ires!=RES_SUCCESS)
                     return ires;
                }
               pdata=pdata->pnext;
            }
         }
         break;
      case OT_ICE_ROLE:
      case OT_ICE_DBUSER:
      case OT_ICE_DBCONNECTION:
      case OT_ICE_SERVER_APPLICATION:
      case OT_ICE_SERVER_LOCATION:
      case OT_ICE_SERVER_VARIABLE:
      case OT_ICE_WEBUSER_ROLE:
      case OT_ICE_WEBUSER_CONNECTION:
      case OT_ICE_PROFILE_ROLE:
      case OT_ICE_PROFILE_CONNECTION:
      case OT_ICE_BUNIT_SEC_ROLE:
      case OT_ICE_BUNIT_SEC_USER:
      case OT_ICE_BUNIT_FACET_ROLE:
      case OT_ICE_BUNIT_FACET_USER:
      case OT_ICE_BUNIT_PAGE_ROLE:
      case OT_ICE_BUNIT_PAGE_USER:
      case OT_ICE_BUNIT_LOCATION:
         {
            struct SimpleLeafObjectData * pdata =
                                      (struct SimpleLeafObjectData * )pstartoflist;

            while (pdata) {
               ires = DBAAppend4Save(fident, pdata,
                                             sizeof(struct SimpleLeafObjectData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pdata=pdata->pnext;
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
                         OTLL_SECURITYALARM 
                   //                 // cache not saved for monitor stuff
                   //      ,OT_MON_REPLIC_SERVER
						};

            while (pDBData) {
			   struct DBData dta = *pDBData;
			   dta.lpMonReplServerData=(struct MonReplServerData  *)0;
               ires = DBAAppend4Save(fident, &dta, sizeof(struct DBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_TABLE:
                         ptemp = (void *) pDBData -> lpTableData      ; break;
                      case OT_VIEW:
                         ptemp = (void *) pDBData -> lpViewData       ; break;
                      case OT_PROCEDURE:
                         ptemp = (void *) pDBData -> lpProcedureData  ; break;
                      case OT_SEQUENCE:
                         ptemp = (void *) pDBData -> lpSequenceData  ; break;
                      case OT_SCHEMAUSER:
                         ptemp = (void *) pDBData -> lpSchemaUserData ; break;
                      case OT_SYNONYM:
                         ptemp = (void *) pDBData -> lpSynonymData    ; break;
                      case OT_DBEVENT:
                         ptemp = (void *) pDBData -> lpDBEventData    ; break;
                      case OT_REPLIC_CONNECTION:
                         ptemp=(void *)pDBData->lpReplicConnectionData; break;
                      case OT_REPLIC_REGTABLE:
                         ptemp= (void *)pDBData ->lpReplicRegTableData; break;
                      case OT_REPLIC_MAILUSER:
                         ptemp= (void *)pDBData -> lpReplicMailErrData; break;
                      case OTLL_REPLIC_CDDS:
                         ptemp =(void *)pDBData -> lpRawReplicCDDSData; break;
                      case OTLL_GRANTEE:
                         ptemp = (void *) pDBData -> lpRawGranteeData ; break;
                      case OTLL_OIDTDBGRANTEE:
                         ptemp = (void *) pDBData ->lpOIDTDBGranteeData;break;
                      case OTLL_SECURITYALARM:
                         ptemp = (void *) pDBData -> lpRawSecurityData; break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pDBData=pDBData->pnext;
            }
         }
         break;
      case OT_NODE :
         {
            LPMONNODEDATA pmonnodedta=(LPMONNODEDATA)pstartoflist;
            int itype[]={OT_NODE_SERVERCLASS,
                         OT_NODE_OPENWIN,
                         OT_NODE_LOGINPSW,
                         OT_NODE_CONNECTIONDATA,
                         OT_NODE_ATTRIBUTE};

            while (pmonnodedta) {
               ires = DBAAppend4Save(fident, pmonnodedta, sizeof(MONNODEDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_NODE_SERVERCLASS:
                         ptemp = (void *) pmonnodedta->lpServerClassData   ;break;
                      case OT_NODE_OPENWIN:
                         ptemp = (void *) pmonnodedta->lpWindowData   ;break;
                      case OT_NODE_LOGINPSW:
                         ptemp = (void *) pmonnodedta->lpNodeLoginData;break;
                      case OT_NODE_CONNECTIONDATA:
                         ptemp = (void *) pmonnodedta->lpNodeConnectionData;break;
                      case OT_NODE_ATTRIBUTE:
                         ptemp = (void *) pmonnodedta->lpNodeAttributeData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pmonnodedta=pmonnodedta->pnext;
            }
         }
         break;
      case OT_NODE_SERVERCLASS :
         {
            LPNODESERVERCLASSDATA pserverclassdta = (LPNODESERVERCLASSDATA)pstartoflist;

            while (pserverclassdta) {
               ires = DBAAppend4Save(fident, pserverclassdta,sizeof(NODESERVERCLASSDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               pserverclassdta=pserverclassdta->pnext;
            }
         }
         break;
      case OT_NODE_OPENWIN  :
         {
            LPNODEWINDOWDATA popenwindta = (LPNODEWINDOWDATA)pstartoflist;

            while (popenwindta) {
               ires = DBAAppend4Save(fident, popenwindta,sizeof(NODEWINDOWDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               popenwindta=popenwindta->pnext;
            }
         }
         break;
      case OT_NODE_LOGINPSW  :
         {
            LPNODELOGINDATA plogindta = (LPNODELOGINDATA)pstartoflist;

            while (plogindta) {
               ires = DBAAppend4Save(fident, plogindta,sizeof(NODELOGINDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               plogindta=plogindta->pnext;
            }
         }
         break;
      case OT_NODE_CONNECTIONDATA:
         {
            LPNODECONNECTDATA pconnectdta = (LPNODECONNECTDATA)pstartoflist;

            while (pconnectdta) {
               ires = DBAAppend4Save(fident, pconnectdta,sizeof(NODECONNECTDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               pconnectdta=pconnectdta->pnext;
            }
         }
         break;
      case OT_NODE_ATTRIBUTE:
         {
            LPNODEATTRIBUTEDATA pattributedta = (LPNODEATTRIBUTEDATA)pstartoflist;

            while (pattributedta) {
               ires = DBAAppend4Save(fident, pattributedta,sizeof(NODEATTRIBUTEDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               pattributedta=pattributedta->pnext;
            }
         }
         break;
      case OT_MON_SERVER     :
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
               ires = DBAAppend4Save(fident, pServerdata,
                                             sizeof(struct ServerData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_SESSION:
                         ptemp = (void *) pServerdata->lpServerSessionData;break;
                      case OT_MON_ICE_CONNECTED_USER:
                         ptemp = (void *) pServerdata->lpIceConnUsersData;break;
                      case OT_MON_ICE_ACTIVE_USER:
                         ptemp = (void *) pServerdata->lpIceActiveUsersData;break;
                      case OT_MON_ICE_TRANSACTION:
                         ptemp = (void *) pServerdata->lpIceTransactionsData;break;
                      case OT_MON_ICE_CURSOR:
                         ptemp = (void *) pServerdata->lpIceCursorsData;break;
                      case OT_MON_ICE_FILEINFO:
                         ptemp = (void *) pServerdata->lpIceCacheInfoData;break;
                      case OT_MON_ICE_DB_CONNECTION:
                         ptemp = (void *) pServerdata->lpIceDbConnectData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pServerdata=pServerdata->pnext;
            }
         }
         break;
      case OT_MON_LOCKLIST     :
         {
            struct LockListData * pLockListdata = (struct LockListData * )pstartoflist;
            int itype[]={OT_MON_LOCK};

            while (pLockListdata) {
               ires = DBAAppend4Save(fident, pLockListdata,
                                             sizeof(struct LockListData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_LOCK:
                         ptemp = (void *) pLockListdata->lpLockData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pLockListdata=pLockListdata->pnext;
            }
         }
         break;
      case OTLL_MON_RESOURCE     :
         {
            struct ResourceData * pResourcedata = (struct ResourceData * )pstartoflist;
            int itype[]={OT_MON_RES_LOCK};

            while (pResourcedata) {
               ires = DBAAppend4Save(fident, pResourcedata,
                                             sizeof(struct ResourceData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_RES_LOCK:
                         ptemp = (void *) pResourcedata->lpResourceLockData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pResourcedata=pResourcedata->pnext;
            }
         }
         break;
      case OT_MON_LOGPROCESS    :
         {
            struct LogProcessData * pLogProcessdata = (struct LogProcessData * )pstartoflist;

            while (pLogProcessdata) {
               ires = DBAAppend4Save(fident, pLogProcessdata,
                                             sizeof(struct LogProcessData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pLogProcessdata=pLogProcessdata->pnext;
            }
         }
         break;
      case OT_MON_LOGDATABASE   :
         {
            struct LogDBData * pLogDBdata = (struct LogDBData * )pstartoflist;

            while (pLogDBdata) {
               ires = DBAAppend4Save(fident, pLogDBdata,
                                             sizeof(struct LogDBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pLogDBdata=pLogDBdata->pnext;
            }
         }
         break;
      case OT_MON_TRANSACTION:
         {
            struct LogTransactData * pLogTransactdata = (struct LogTransactData * )pstartoflist;

            while (pLogTransactdata) {
               ires = DBAAppend4Save(fident, pLogTransactdata,
                                             sizeof(struct LogTransactData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pLogTransactdata=pLogTransactdata->pnext;
            }
         }
         break;
      case OT_MON_RES_LOCK :
      case OT_MON_LOCK :
         {
            struct LockData * pLockdata = (struct LockData * )pstartoflist;

            while (pLockdata) {
               ires = DBAAppend4Save(fident, pLockdata,
                                             sizeof(struct LockData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pLockdata=pLockdata->pnext;
            }
         }
         break;
      case OT_MON_SESSION :
         {
            struct SessionData * pSessiondata = (struct SessionData * )pstartoflist;

            while (pSessiondata) {
               ires = DBAAppend4Save(fident, pSessiondata,
                                             sizeof(struct SessionData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pSessiondata=pSessiondata->pnext;
            }
         }
         break;
      case OT_MON_ICE_CONNECTED_USER :
         {
            struct IceConnUserData * pIceConnectuserdata = (struct IceConnUserData * )pstartoflist;

            while (pIceConnectuserdata) {
               ires = DBAAppend4Save(fident, pIceConnectuserdata,
                                             sizeof(struct IceConnUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceConnectuserdata=pIceConnectuserdata->pnext;
            }
         }
         break;
      case OT_MON_ICE_ACTIVE_USER :
         {
            struct IceActiveUserData * pIceActiveUserdata = (struct IceActiveUserData * )pstartoflist;

            while (pIceActiveUserdata) {
               ires = DBAAppend4Save(fident, pIceActiveUserdata,
                                             sizeof(struct IceActiveUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceActiveUserdata=pIceActiveUserdata->pnext;
            }
         }
         break;
      case OT_MON_ICE_TRANSACTION :
         {
            struct IceTransactData * pIceTransactdata = (struct IceTransactData * )pstartoflist;

            while (pIceTransactdata) {
               ires = DBAAppend4Save(fident, pIceTransactdata,
                                             sizeof(struct IceTransactData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceTransactdata=pIceTransactdata->pnext;
            }
         }
         break;
      case OT_MON_ICE_CURSOR :
         {
            struct IceCursorData * pIceCursordata = (struct IceCursorData * )pstartoflist;

            while (pIceCursordata) {
               ires = DBAAppend4Save(fident, pIceCursordata,
                                             sizeof(struct IceCursorData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceCursordata=pIceCursordata->pnext;
            }
         }
         break;
      case OT_MON_ICE_FILEINFO :
         {
            struct IceCacheData * pIceCachedata = (struct IceCacheData * )pstartoflist;

            while (pIceCachedata) {
               ires = DBAAppend4Save(fident, pIceCachedata,
                                             sizeof(struct IceCacheData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceCachedata=pIceCachedata->pnext;
            }
         }
         break;
      case OT_MON_ICE_DB_CONNECTION :
         {
            struct IceDbConnectData * pIceDbConnectdata = (struct IceDbConnectData * )pstartoflist;

            while (pIceDbConnectdata) {
               ires = DBAAppend4Save(fident, pIceDbConnectdata,
                                             sizeof(struct IceDbConnectData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIceDbConnectdata=pIceDbConnectdata->pnext;
            }
         }
         break;

      case OT_PROFILE   :
         {
            struct ProfileData * pProfiledata = (struct ProfileData * )pstartoflist;

            while (pProfiledata) {
               ires = DBAAppend4Save(fident, pProfiledata,
                                             sizeof(struct ProfileData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pProfiledata=pProfiledata->pnext;
            }
         }
         break;
      case OT_USER      :
         {
            struct UserData * puserdata = (struct UserData * )pstartoflist;

            while (puserdata) {
               ires = DBAAppend4Save(fident, puserdata,
                                             sizeof(struct UserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               puserdata=puserdata->pnext;
            }
         }
         break;
      case OT_GROUP     :
         {
            struct GroupData * pGroupdata = (struct GroupData * )pstartoflist;
            int itype[]={OT_GROUPUSER };

            while (pGroupdata) {
               ires = DBAAppend4Save(fident, pGroupdata,
                                             sizeof(struct GroupData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_GROUPUSER:
                         ptemp = (void *) pGroupdata->lpGroupUserData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pGroupdata=pGroupdata->pnext;
            }
         }
         break;
      case OT_GROUPUSER :
         {
            struct GroupUserData * pgroupuserdata = (struct GroupUserData * )pstartoflist;

            while (pgroupuserdata) {
               ires = DBAAppend4Save(fident, pgroupuserdata,
                                             sizeof(struct GroupUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pgroupuserdata=pgroupuserdata->pnext;
            }
         }
         break;
      case OT_ROLE      :
         {
            struct RoleData * pRoledata = (struct RoleData * )pstartoflist;
            int itype[]={OT_ROLEGRANT_USER };

            while (pRoledata) {
               ires = DBAAppend4Save(fident, pRoledata,
                                             sizeof(struct RoleData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ROLEGRANT_USER:
                         ptemp = (void *) pRoledata->lpRoleGranteeData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pRoledata=pRoledata->pnext;
            }
         }
         break;
      case OT_ROLEGRANT_USER :
         {
            struct RoleGranteeData * pRoleGranteedata = (struct RoleGranteeData * )pstartoflist;

            while (pRoleGranteedata) {
               ires = DBAAppend4Save(fident, pRoleGranteedata,
                                             sizeof(struct RoleGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRoleGranteedata=pRoleGranteedata->pnext;
            }
         }
         break;
      case OT_LOCATION  :
         {
            struct LocationData * pLocationdata = (struct LocationData * )pstartoflist;

            while (pLocationdata) {
               ires = DBAAppend4Save(fident, pLocationdata,
                                             sizeof(struct LocationData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pLocationdata=pLocationdata->pnext;
            }
         }
         break;
      case OT_TABLE     :
         {
            struct TableData * pTableData = (struct TableData * )pstartoflist;
            int itype[]={OT_INTEGRITY,
                         OT_COLUMN,
                         OT_RULE,
                         OT_INDEX,
                         OT_TABLELOCATION };

            while (pTableData) {
               ires = DBAAppend4Save(fident, pTableData, sizeof(struct TableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_INTEGRITY:
                         ptemp = (void *) pTableData->lpIntegrityData;break;
                      case OT_COLUMN:
                         ptemp = (void *) pTableData->lpColumnData   ;break;
                      case OT_RULE:
                         ptemp = (void *) pTableData->lpRuleData     ;break;
                      case OT_INDEX:
                         ptemp = (void *) pTableData->lpIndexData    ;break;
                      case OT_TABLELOCATION:
                         ptemp =(void *)pTableData->lpTableLocationData;break;
                      default:
                         return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
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
               ires = DBAAppend4Save(fident, pViewData, sizeof(struct ViewData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                  switch (itype[i]){
                     case OT_VIEWTABLE:
                        ptemp = (void *) pViewData->lpViewTableData;break;
                     case OT_VIEWCOLUMN:
                        ptemp = (void *) pViewData->lpColumnData   ;break;
                     default:
                        return RES_ERR;
                  }
                  ires= DOMSaveList(fident, hnodestruct, itype[i], ptemp);
                  if (ires!=RES_SUCCESS)
                     return ires;
               }
               pViewData=pViewData->pnext;
            }
         }
         break;
      case OT_INTEGRITY :
         {
            struct IntegrityData * pIntegrityData = (struct IntegrityData * )pstartoflist;

            while (pIntegrityData) {
               ires = DBAAppend4Save(fident, pIntegrityData, sizeof(struct IntegrityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIntegrityData=pIntegrityData->pnext;
            }
         }
         break;
      case OT_COLUMN :
      case OT_VIEWCOLUMN :
         {
            struct ColumnData * pColumndata = (struct ColumnData * )pstartoflist;

            while (pColumndata) {
               ires = DBAAppend4Save(fident, pColumndata,
                                             sizeof(struct ColumnData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pColumndata=pColumndata->pnext;
            }
         }
         break;
      case OT_RULE   :
         {
            struct RuleData * pRuledata = (struct RuleData * )pstartoflist;

            while (pRuledata) {
               ires = DBAAppend4Save(fident, pRuledata,
                                             sizeof(struct RuleData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRuledata=pRuledata->pnext;
            }
         }
         break;
      case OT_INDEX                      :
         {
            struct IndexData * pIndexData = (struct IndexData * )pstartoflist;

            while (pIndexData) {
               ires = DBAAppend4Save(fident, pIndexData, sizeof(struct IndexData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pIndexData=pIndexData->pnext;
            }
         }
         break;
      case OT_TABLELOCATION              :
         {
            struct LocationData * pTableLocationData = (struct LocationData * )pstartoflist;

            while (pTableLocationData) {
               ires = DBAAppend4Save(fident, pTableLocationData, sizeof(struct LocationData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pTableLocationData=pTableLocationData->pnext;
            }
         }
         break;
      case OT_PROCEDURE                  :
         {
            struct ProcedureData * pProceduredata =
                                       (struct ProcedureData * )pstartoflist;

            while (pProceduredata) {
               ires = DBAAppend4Save(fident, pProceduredata,
                                             sizeof(struct ProcedureData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pProceduredata=pProceduredata->pnext;
            }
         }
         break;
      case OT_SEQUENCE:
         {
            struct SequenceData * pSequencedata =
                                       (struct SequenceData * )pstartoflist;

            while (pSequencedata) {
               ires = DBAAppend4Save(fident, pSequencedata,
                                             sizeof(struct SequenceData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pSequencedata=pSequencedata->pnext;
            }
         }
         break;
      case OT_SCHEMAUSER                 :
         {
            struct SchemaUserData * pSchemaUserdata =
                                       (struct SchemaUserData * )pstartoflist;

            while (pSchemaUserdata) {
               ires = DBAAppend4Save(fident, pSchemaUserdata,
                                             sizeof(struct SchemaUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pSchemaUserdata=pSchemaUserdata->pnext;
            }
         }
         break;
      case OT_SYNONYM                    :
         {
            struct SynonymData * pSynonymdata =
                                       (struct SynonymData * )pstartoflist;

            while (pSynonymdata) {
               ires = DBAAppend4Save(fident, pSynonymdata,
                                             sizeof(struct SynonymData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pSynonymdata=pSynonymdata->pnext;
            }
         }
         break;
      case OT_DBEVENT                    :
         {
            struct DBEventData * pDBEventdata =
                                       (struct DBEventData * )pstartoflist;

            while (pDBEventdata) {
               ires = DBAAppend4Save(fident, pDBEventdata,
                                             sizeof(struct DBEventData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pDBEventdata=pDBEventdata->pnext;
            }
         }
         break;
      case OT_REPLIC_CONNECTION           :
         {
            struct ReplicConnectionData * pReplicConnectiondata =
                                       (struct ReplicConnectionData * )pstartoflist;

            while (pReplicConnectiondata) {
               ires = DBAAppend4Save(fident, pReplicConnectiondata,
                                             sizeof(struct ReplicConnectionData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pReplicConnectiondata=pReplicConnectiondata->pnext;
            }
         }
         break;
      case OT_REPLIC_REGTABLE :
         {
            struct ReplicRegTableData * pReplicRegTabledata =
                                       (struct ReplicRegTableData * )pstartoflist;

            while (pReplicRegTabledata) {
               ires = DBAAppend4Save(fident, pReplicRegTabledata,
                                             sizeof(struct ReplicRegTableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pReplicRegTabledata=pReplicRegTabledata->pnext;
            }
         }
         break;
      case OT_REPLIC_MAILUSER :
         {
            struct ReplicMailErrData * pReplicMailErrdata =
                                       (struct ReplicMailErrData * )pstartoflist;

            while (pReplicMailErrdata) {
               ires = DBAAppend4Save(fident, pReplicMailErrdata,
                                             sizeof(struct ReplicMailErrData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pReplicMailErrdata=pReplicMailErrdata->pnext;
            }
         }
         break;
      case OTLL_REPLIC_CDDS :
         {
            struct RawReplicCDDSData * pRawReplicCDDSdata =
                                       (struct RawReplicCDDSData * )pstartoflist;

            while (pRawReplicCDDSdata) {
               ires = DBAAppend4Save(fident, pRawReplicCDDSdata,
                                             sizeof(struct RawReplicCDDSData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawReplicCDDSdata=pRawReplicCDDSdata->pnext;
            }
         }
         break;
      case OT_VIEWTABLE :
         {
            struct ViewTableData * pViewTabledata =
                                       (struct ViewTableData * )pstartoflist;

            while (pViewTabledata) {
               ires = DBAAppend4Save(fident, pViewTabledata,
                                             sizeof(struct ViewTableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pViewTabledata=pViewTabledata->pnext;
            }
         }
         break;
      case OTLL_DBGRANTEE :
         {
            struct RawDBGranteeData * pRawDBGranteedata =
                                       (struct RawDBGranteeData * )pstartoflist;

            while (pRawDBGranteedata) {
               ires = DBAAppend4Save(fident, pRawDBGranteedata,
                                             sizeof(struct RawDBGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawDBGranteedata=pRawDBGranteedata->pnext;
            }
         }
         break;
      case OTLL_DBSECALARM :
         {
            struct RawSecurityData * pRawDBSecuritydata =
                                       (struct RawSecurityData * )pstartoflist;

            while (pRawDBSecuritydata) {
               ires = DBAAppend4Save(fident, pRawDBSecuritydata,
                                             sizeof(struct RawSecurityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawDBSecuritydata=pRawDBSecuritydata->pnext;
            }
         }
         break;
      case OTLL_GRANTEE :
         {
            struct RawGranteeData * pRawGranteedata =
                                       (struct RawGranteeData * )pstartoflist;

            while (pRawGranteedata) {
               ires = DBAAppend4Save(fident, pRawGranteedata,
                                             sizeof(struct RawGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawGranteedata=pRawGranteedata->pnext;
            }
         }
         break;
      case OTLL_OIDTDBGRANTEE :
         {
            struct RawDBGranteeData * pRawDBGranteedata =
                                       (struct RawDBGranteeData * )pstartoflist;

            while (pRawDBGranteedata) {
               ires = DBAAppend4Save(fident, pRawDBGranteedata,
                                             sizeof(struct RawDBGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawDBGranteedata=pRawDBGranteedata->pnext;
            }
         }
         break;
      case OTLL_SECURITYALARM :
         {
            struct RawSecurityData * pRawSecuritydata =
                                       (struct RawSecurityData * )pstartoflist;

            while (pRawSecuritydata) {
               ires = DBAAppend4Save(fident, pRawSecuritydata,
                                             sizeof(struct RawSecurityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               pRawSecuritydata=pRawSecuritydata->pnext;
            }
         }
         break;
      default:
         {
            char toto[40];
            // type of error1 %d
            wsprintf(toto, ResourceString ((UINT)IDS_E_TYPEOF_ERROR1), iobjecttype);
            TS_MessageBox( TS_GetFocus(), toto, NULL, MB_OK | MB_TASKMODAL);
            return myerror(ERR_OBJECTNOEXISTS);
         }
         break;
   }
   return ires;
}

int  DOMReadList ( fident, hnodestruct, iobjecttype, lplpstartoflist)
FILEIDENT fident;
int hnodestruct;
int iobjecttype;
void ** lplpstartoflist;
{
   int   i, ires, ires1;
   BOOL  btemp;
   void ** pptemp;
   struct ServConnectData * pdata1 = virtnode[hnodestruct].pdata;

   ires = RES_SUCCESS;

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
                   //     ,               // cache not saved for monitor stuff
                    //     OT_MON_SERVER,
                    //     OT_MON_LOCKLIST,
                    //     OTLL_MON_RESOURCE,
                    //     OT_MON_LOGPROCESS,    
                    //     OT_MON_LOGDATABASE,   
                    //     OT_MON_TRANSACTION
                         OT_ICE_ROLE,
                         OT_ICE_DBUSER,
                         OT_ICE_DBCONNECTION,
                         OT_ICE_WEBUSER,
                         OT_ICE_PROFILE,
                         OT_ICE_BUNIT,
                         OT_ICE_SERVER_APPLICATION,
                         OT_ICE_SERVER_LOCATION,
                         OT_ICE_SERVER_VARIABLE
                        
                        };
            virtnode[hnodestruct].pdata = (struct ServConnectData *)0;
            ires = DBAReadData(fident, &btemp, sizeof(btemp));
            if (ires!=RES_SUCCESS)
               return ires;
            if (!btemp) 
               return RES_SUCCESS;

            ires = DBAReadData(fident, &virtnode[hnodestruct].nUsers,
                                  sizeof(virtnode[hnodestruct].nUsers));
            if (ires!=RES_SUCCESS)
               return ires;

            pdata1 = (struct ServConnectData *)
                       ESL_AllocMem( (UINT) sizeof(struct ServConnectData));
            if (!pdata1)
               return RES_ERR;

            virtnode[hnodestruct].bIsSpecialMonState=TRUE; // special state
            virtnode[hnodestruct].pdata = pdata1;

            ires = DBAReadData(fident,pdata1,sizeof(struct ServConnectData));
            if (ires!=RES_SUCCESS)
               return ires;

            pdata1->lpServerData      = NULL;   // cache not saved for monitor info
            pdata1->lpLockListData    = NULL;   // (too dynamic data)
            pdata1->lpResourceData    = NULL;
            pdata1->lpLogProcessData  = NULL;
            pdata1->lpLogDBData       = NULL;
            pdata1->lpLogTransactData = NULL;

            for (i=0;i<sizeof(itype)/sizeof(int);i++) {
               switch (itype[i]){
                  case OT_DATABASE:
                     pptemp = (void **) &(pdata1->lpDBData)          ; break;
                  case OT_PROFILE:
                     pptemp = (void **) &(pdata1->lpProfilesData)    ; break;
                  case OT_USER:
                     pptemp = (void **) &(pdata1->lpUsersData)       ; break;
                  case OT_GROUP:
                     pptemp = (void **) &(pdata1->lpGroupsData)      ; break;
                  case OT_ROLE:
                     pptemp = (void **) &(pdata1->lpRolesData)       ; break;
                  case OT_LOCATION:
                     pptemp = (void **) &(pdata1->lpLocationsData)   ; break;
                  case OTLL_DBSECALARM:
                     pptemp = (void **) &(pdata1->lpRawDBSecurityData); break;
                  case OTLL_DBGRANTEE:
                     pptemp = (void **) &(pdata1->lpRawDBGranteeData); break;
                  case OT_MON_SERVER        :
                     pptemp = (void **) &(pdata1->lpServerData)      ; break;
                  case OT_MON_LOCKLIST      :
                     pptemp = (void **) &(pdata1->lpLockListData)    ; break;
                  case OTLL_MON_RESOURCE      :
                     pptemp = (void **) &(pdata1->lpResourceData)    ; break;
                  case OT_MON_LOGPROCESS    :
                     pptemp = (void **) &(pdata1->lpLogProcessData)  ; break;
                  case OT_MON_LOGDATABASE   :
                     pptemp = (void **) &(pdata1->lpLogDBData)       ; break;
                  case OT_MON_TRANSACTION:
                     pptemp = (void **) &(pdata1->lpLogTransactData) ; break;
                  case OT_ICE_ROLE:
                     pptemp = (void **) &(pdata1->lpIceRolesData) ; break;
                  case OT_ICE_DBUSER:
                     pptemp = (void **) &(pdata1->lpIceDbUsersData) ; break;
                  case OT_ICE_DBCONNECTION:
                     pptemp = (void **) &(pdata1->lpIceDbConnectionsData) ; break;
                  case OT_ICE_WEBUSER:
                     pptemp = (void **) &(pdata1->lpIceWebUsersData) ; break;
                  case OT_ICE_PROFILE:
                     pptemp = (void **) &(pdata1->lpIceProfilesData) ; break;
                  case OT_ICE_BUNIT:
                     pptemp = (void **) &(pdata1->lpIceBusinessUnitsData) ; break;
                  case OT_ICE_SERVER_APPLICATION:
                     pptemp = (void **) &(pdata1->lpIceApplicationsData) ; break;
                  case OT_ICE_SERVER_LOCATION:
                     pptemp = (void **) &(pdata1->lpIceLocationsData) ; break;
                  case OT_ICE_SERVER_VARIABLE:
                     pptemp = (void **) &(pdata1->lpIceVarsData) ; break;
                    default:
                     return RES_ERR;
               }
               ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
               if (ires1!=RES_SUCCESS)
                  ires=ires1;
            }
            if (ires!=RES_SUCCESS) {
               virtnode[hnodestruct].pdata = (struct ServConnectData *)0;
               // TO BE FINISHED: free linked lists
            }
         }
         break;
      case OT_ICE_WEBUSER:
      case OT_ICE_PROFILE:
         {
            struct IceObjectWithRolesAndDBData ** ppdata =
                                      (struct IceObjectWithRolesAndDBData ** )lplpstartoflist;
            int itype[]={OT_ICE_WEBUSER_ROLE,
                         OT_ICE_WEBUSER_CONNECTION };
			if (iobjecttype==OT_ICE_PROFILE) {
				itype[0]=OT_ICE_PROFILE_ROLE;
				itype[1]=OT_ICE_PROFILE_CONNECTION;
			};

            while (*ppdata) {
               *ppdata = ESL_AllocMem(sizeof(struct IceObjectWithRolesAndDBData));
               if (!*ppdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppdata,
                                             sizeof(struct IceObjectWithRolesAndDBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_WEBUSER_ROLE:
                      case OT_ICE_PROFILE_ROLE:
                         pptemp = (void **)&((*ppdata)->lpObjectRolesData);break;
                      case OT_ICE_WEBUSER_CONNECTION:
                      case OT_ICE_PROFILE_CONNECTION:
                         pptemp = (void **)&((*ppdata)->lpObjectDbData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppdata=&((*ppdata)->pnext);
            }
         }
         break;
      case OT_ICE_BUNIT:
         {
            struct IceBusinessUnitsData ** ppdata =
                                      (struct IceBusinessUnitsData ** )lplpstartoflist;
            int itype[]={OT_ICE_BUNIT_SEC_ROLE,
                         OT_ICE_BUNIT_SEC_USER,
                         OT_ICE_BUNIT_FACET,
                         OT_ICE_BUNIT_PAGE,
                         OT_ICE_BUNIT_LOCATION};
            while (*ppdata) {
               *ppdata = ESL_AllocMem(sizeof(struct IceBusinessUnitsData));
               if (!*ppdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppdata,
                                             sizeof(struct IceBusinessUnitsData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_SEC_ROLE:
                         pptemp = (void **)&((*ppdata)->lpObjectRolesData);break;
                      case OT_ICE_BUNIT_SEC_USER:
                         pptemp = (void **)&((*ppdata)->lpObjectUsersData);break;
                      case OT_ICE_BUNIT_FACET:
                         pptemp = (void **)&((*ppdata)->lpObjectFacetsData);break;
                      case OT_ICE_BUNIT_PAGE:
                         pptemp = (void **)&((*ppdata)->lpObjectPagesData);break;
                      case OT_ICE_BUNIT_LOCATION:
                         pptemp = (void **)&((*ppdata)->lpObjectLocationsData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppdata=&((*ppdata)->pnext);
            }
         }
         break;
      case OT_ICE_BUNIT_FACET:
      case OT_ICE_BUNIT_PAGE:
         {
            struct IceObjectWithRolesAndUsersData ** ppdata =
                                      (struct IceObjectWithRolesAndUsersData ** )lplpstartoflist;
            int itype[]={OT_ICE_BUNIT_PAGE_ROLE,
                         OT_ICE_BUNIT_PAGE_USER };
			if (iobjecttype==OT_ICE_BUNIT_FACET) {
				itype[0]=OT_ICE_BUNIT_FACET_ROLE;
                itype[1]=OT_ICE_BUNIT_FACET_USER;
			}

            while (*ppdata) {
               *ppdata = ESL_AllocMem(sizeof(struct IceObjectWithRolesAndUsersData));
               if (!*ppdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppdata,
                                             sizeof(struct IceObjectWithRolesAndUsersData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ICE_BUNIT_PAGE_ROLE:
                      case OT_ICE_BUNIT_FACET_ROLE:
                         pptemp = (void **)&((*ppdata)->lpObjectRolesData);break;
                      case OT_ICE_BUNIT_PAGE_USER:
                      case OT_ICE_BUNIT_FACET_USER:
                         pptemp = (void **)&((*ppdata)->lpObjectUsersData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppdata=&((*ppdata)->pnext);
            }
         }
         break;
      case OT_ICE_ROLE:
      case OT_ICE_DBUSER:
      case OT_ICE_DBCONNECTION:
      case OT_ICE_SERVER_APPLICATION:
      case OT_ICE_SERVER_LOCATION:
      case OT_ICE_SERVER_VARIABLE:
      case OT_ICE_WEBUSER_ROLE:
      case OT_ICE_WEBUSER_CONNECTION:
      case OT_ICE_PROFILE_ROLE:
      case OT_ICE_PROFILE_CONNECTION:
      case OT_ICE_BUNIT_SEC_ROLE:
      case OT_ICE_BUNIT_SEC_USER:
      case OT_ICE_BUNIT_FACET_ROLE:
      case OT_ICE_BUNIT_FACET_USER:
      case OT_ICE_BUNIT_PAGE_ROLE:
      case OT_ICE_BUNIT_PAGE_USER:
      case OT_ICE_BUNIT_LOCATION:
         {
            struct SimpleLeafObjectData ** ppdata =
                                      (struct SimpleLeafObjectData ** )lplpstartoflist;

            while (*ppdata) {
               *ppdata = ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
               if (!*ppdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppdata,
                                             sizeof(struct SimpleLeafObjectData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppdata=&((*ppdata)->pnext);
            }
         }
         break;
      case OT_DATABASE  :
         {
            struct DBData ** ppDBData = (struct DBData ** )lplpstartoflist;
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
                         OTLL_SECURITYALARM };


            while (*ppDBData) {
               *ppDBData = ESL_AllocMem(sizeof(struct DBData));
               if (!*ppDBData)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppDBData,
                                          sizeof(struct DBData));
               if (ires!=RES_SUCCESS)
                  return ires;
			   (*ppDBData)->lpMonReplServerData=NULL;
			   (*ppDBData)->nbmonreplservers=0;
			   for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_TABLE:
                         pptemp = (void **)&((*ppDBData)->lpTableData      );break;
                      case OT_VIEW:
                         pptemp = (void **)&((*ppDBData)->lpViewData       );break;
                      case OT_PROCEDURE:
                         pptemp = (void **)&((*ppDBData)->lpProcedureData  );break;
                      case OT_SEQUENCE:
                         pptemp = (void **)&((*ppDBData)->lpSequenceData   );break;
                      case OT_SCHEMAUSER:
                         pptemp = (void **)&((*ppDBData)->lpSchemaUserData );break;
                      case OT_SYNONYM:
                         pptemp = (void **)&((*ppDBData)->lpSynonymData    );break;
                      case OT_DBEVENT:
                         pptemp = (void **)&((*ppDBData)->lpDBEventData    );break;
                      case OT_REPLIC_CONNECTION:
                         pptemp=(void **)&((*ppDBData)->lpReplicConnectionData);break;
                         case OT_REPLIC_REGTABLE:
                         pptemp= (void **)&((*ppDBData)->lpReplicRegTableData);break;
                      case OT_REPLIC_MAILUSER:
                         pptemp= (void **)&((*ppDBData)->lpReplicMailErrData);break;
                      case OTLL_REPLIC_CDDS:
                         pptemp =(void **)&((*ppDBData)->lpRawReplicCDDSData);break;
                      case OTLL_GRANTEE:
                         pptemp = (void **)&((*ppDBData)->lpRawGranteeData);break;
                      case OTLL_OIDTDBGRANTEE:
                         pptemp = (void **)&((*ppDBData)->lpOIDTDBGranteeData);break;
                      case OTLL_SECURITYALARM:
                         pptemp = (void **)&((*ppDBData)->lpRawSecurityData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppDBData=&((*ppDBData)->pnext);
            }
         }
         break;
      case OT_NODE :
         {
            LPMONNODEDATA * ppmonnodedta=(LPMONNODEDATA *)lplpstartoflist;
            int itype[]={OT_NODE_SERVERCLASS,
                         OT_NODE_OPENWIN,
                         OT_NODE_LOGINPSW,
                         OT_NODE_CONNECTIONDATA,
                         OT_NODE_ATTRIBUTE};
            while (*ppmonnodedta) {
               *ppmonnodedta = ESL_AllocMem(sizeof(MONNODEDATA));
               if (!*ppmonnodedta)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppmonnodedta,
                                          sizeof(MONNODEDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_NODE_SERVERCLASS:
                         pptemp = (void **)&((*ppmonnodedta)->lpServerClassData);break;
                      case OT_NODE_OPENWIN:
                         pptemp = (void **)&((*ppmonnodedta)->lpWindowData)   ;break;
                      case OT_NODE_LOGINPSW:
                         pptemp = (void **)&((*ppmonnodedta)->lpNodeLoginData);break;
                      case OT_NODE_CONNECTIONDATA:
                         pptemp = (void **)&((*ppmonnodedta)->lpNodeConnectionData);break;
                      case OT_NODE_ATTRIBUTE:
                         pptemp = (void **)&((*ppmonnodedta)->lpNodeAttributeData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppmonnodedta=&((*ppmonnodedta)->pnext);
            }
         }
         break;
      case OT_NODE_SERVERCLASS  :
         {
            LPNODESERVERCLASSDATA * ppserverclassdata = (LPNODESERVERCLASSDATA *)lplpstartoflist;
            while (*ppserverclassdata) {
               *ppserverclassdata = ESL_AllocMem(sizeof(NODESERVERCLASSDATA));
               if (!*ppserverclassdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppserverclassdata, sizeof(NODESERVERCLASSDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppserverclassdata=&((*ppserverclassdata)->pnext);
            }
         }
         break;
      case OT_NODE_OPENWIN  :
         {
            LPNODEWINDOWDATA * ppwnddata = (LPNODEWINDOWDATA *)lplpstartoflist;
            while (*ppwnddata) {
               *ppwnddata = ESL_AllocMem(sizeof(NODEWINDOWDATA));
               if (!*ppwnddata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppwnddata, sizeof(NODEWINDOWDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppwnddata=&((*ppwnddata)->pnext);
            }
         }
         break;
      case OT_NODE_LOGINPSW  :
         {
            LPNODELOGINDATA * pplogindata = (LPNODELOGINDATA *)lplpstartoflist;
            while (*pplogindata) {
               *pplogindata = ESL_AllocMem(sizeof(NODELOGINDATA));
               if (!*pplogindata)
                  return RES_ERR;
               ires = DBAReadData(fident, *pplogindata, sizeof(NODELOGINDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               pplogindata=&((*pplogindata)->pnext);
            }
         }
         break;
      case OT_NODE_CONNECTIONDATA  :
         {
            LPNODECONNECTDATA * ppconnectdata = (LPNODECONNECTDATA *)lplpstartoflist;
            while (*ppconnectdata) {
               *ppconnectdata = ESL_AllocMem(sizeof(NODECONNECTDATA));
               if (!*ppconnectdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppconnectdata, sizeof(NODECONNECTDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppconnectdata=&((*ppconnectdata)->pnext);
            }
         }
         break;
      case OT_NODE_ATTRIBUTE  :
         {
            LPNODEATTRIBUTEDATA * ppattributedata = (LPNODEATTRIBUTEDATA *)lplpstartoflist;
            while (*ppattributedata) {
               *ppattributedata = ESL_AllocMem(sizeof(NODEATTRIBUTEDATA));
               if (!*ppattributedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppattributedata, sizeof(NODEATTRIBUTEDATA));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppattributedata=&((*ppattributedata)->pnext);
            }
         }
         break;
      case OT_MON_SERVER     :
         {
            struct ServerData ** ppServerdata =
                                      (struct ServerData ** )lplpstartoflist;
            int itype[]={OT_MON_SESSION,
                         OT_MON_ICE_CONNECTED_USER,
                         OT_MON_ICE_ACTIVE_USER,
                         OT_MON_ICE_TRANSACTION,
                         OT_MON_ICE_CURSOR,
                         OT_MON_ICE_FILEINFO,
                         OT_MON_ICE_DB_CONNECTION};

            while (*ppServerdata) {
               *ppServerdata = ESL_AllocMem(sizeof(struct ServerData));
               if (!*ppServerdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppServerdata,
                                             sizeof(struct ServerData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_SESSION:
                         pptemp = (void **)&((*ppServerdata)->lpServerSessionData);break;
                      case OT_MON_ICE_CONNECTED_USER:
                         pptemp = (void **)&((*ppServerdata)->lpIceConnUsersData);break;
                      case OT_MON_ICE_ACTIVE_USER:
                         pptemp = (void **)&((*ppServerdata)->lpIceActiveUsersData);break;
                      case OT_MON_ICE_TRANSACTION:
                         pptemp = (void **)&((*ppServerdata)->lpIceTransactionsData);break;
                      case OT_MON_ICE_CURSOR:
                         pptemp = (void **)&((*ppServerdata)->lpIceCursorsData);break;
                      case OT_MON_ICE_FILEINFO:
                         pptemp = (void **)&((*ppServerdata)->lpIceCacheInfoData);break;
                      case OT_MON_ICE_DB_CONNECTION:
                         pptemp = (void **)&((*ppServerdata)->lpIceDbConnectData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppServerdata=&((*ppServerdata)->pnext);
            }
         }
         break;
      case OT_MON_LOCKLIST     :
         {
            struct LockListData ** ppLockListdata =
                                      (struct LockListData ** )lplpstartoflist;
            int itype[]={OT_MON_LOCK };

            while (*ppLockListdata) {
               *ppLockListdata = ESL_AllocMem(sizeof(struct LockListData));
               if (!*ppLockListdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLockListdata,
                                             sizeof(struct LockListData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_LOCK:
                         pptemp = (void **)&((*ppLockListdata)->lpLockData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppLockListdata=&((*ppLockListdata)->pnext);
            }
         }
         break;
      case OTLL_MON_RESOURCE     :
         {
            struct ResourceData ** ppResourcedata =
                                      (struct ResourceData ** )lplpstartoflist;
            int itype[]={OT_MON_RES_LOCK};

            while (*ppResourcedata) {
               *ppResourcedata = ESL_AllocMem(sizeof(struct ResourceData));
               if (!*ppResourcedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppResourcedata,
                                             sizeof(struct ResourceData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_MON_RES_LOCK:
                         pptemp = (void **)&((*ppResourcedata)->lpResourceLockData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppResourcedata=&((*ppResourcedata)->pnext);
            }
         }
         break;
      case OT_MON_SESSION :
         {
            struct SessionData ** ppSessiondata =
                                        (struct SessionData ** )lplpstartoflist;

            while (*ppSessiondata) {
               *ppSessiondata = ESL_AllocMem(sizeof(struct SessionData));
               if (!*ppSessiondata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppSessiondata,
                                             sizeof(struct SessionData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppSessiondata=&((*ppSessiondata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_CONNECTED_USER :
         {
            struct IceConnUserData ** ppIceConnUserdata =
                                        (struct IceConnUserData ** )lplpstartoflist;

            while (*ppIceConnUserdata) {
               *ppIceConnUserdata = ESL_AllocMem(sizeof(struct IceConnUserData));
               if (!*ppIceConnUserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceConnUserdata,
                                             sizeof(struct IceConnUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceConnUserdata=&((*ppIceConnUserdata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_ACTIVE_USER :
         {
            struct IceActiveUserData ** ppIceActiveUserdata =
                                        (struct IceActiveUserData ** )lplpstartoflist;

            while (*ppIceActiveUserdata) {
               *ppIceActiveUserdata = ESL_AllocMem(sizeof(struct IceActiveUserData));
               if (!*ppIceActiveUserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceActiveUserdata,
                                             sizeof(struct IceActiveUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceActiveUserdata=&((*ppIceActiveUserdata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_TRANSACTION :
         {
            struct IceTransactData ** ppIceTransactdata =
                                        (struct IceTransactData ** )lplpstartoflist;

            while (*ppIceTransactdata) {
               *ppIceTransactdata = ESL_AllocMem(sizeof(struct IceTransactData));
               if (!*ppIceTransactdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceTransactdata,
                                             sizeof(struct IceTransactData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceTransactdata=&((*ppIceTransactdata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_CURSOR :
         {
            struct IceCursorData ** ppIceCursordata =
                                        (struct IceCursorData ** )lplpstartoflist;

            while (*ppIceCursordata) {
               *ppIceCursordata = ESL_AllocMem(sizeof(struct IceCursorData));
               if (!*ppIceCursordata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceCursordata,
                                             sizeof(struct IceCursorData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceCursordata=&((*ppIceCursordata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_FILEINFO :
         {
            struct IceCacheData ** ppIceCachedata =
                                        (struct IceCacheData ** )lplpstartoflist;

            while (*ppIceCachedata) {
               *ppIceCachedata = ESL_AllocMem(sizeof(struct IceCacheData));
               if (!*ppIceCachedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceCachedata,
                                             sizeof(struct IceCacheData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceCachedata=&((*ppIceCachedata)->pnext);
            }
         }
         break;
      case OT_MON_ICE_DB_CONNECTION :
         {
            struct IceDbConnectData ** ppIceDbConnectdata =
                                        (struct IceDbConnectData ** )lplpstartoflist;

            while (*ppIceDbConnectdata) {
               *ppIceDbConnectdata = ESL_AllocMem(sizeof(struct IceDbConnectData));
               if (!*ppIceDbConnectdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIceDbConnectdata,
                                             sizeof(struct IceDbConnectData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIceDbConnectdata=&((*ppIceDbConnectdata)->pnext);
            }
         }
         break;
      case OT_MON_LOCK :
      case OT_MON_RES_LOCK :
         {
            struct LockData ** ppLockdata =
                                        (struct LockData ** )lplpstartoflist;

            while (*ppLockdata) {
               *ppLockdata = ESL_AllocMem(sizeof(struct LockData));
               if (!*ppLockdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLockdata,
                                             sizeof(struct LockData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppLockdata=&((*ppLockdata)->pnext);
            }
         }
         break;
      case OT_MON_LOGPROCESS      :
         {
            struct LogProcessData ** ppLogProcessdata =
                                      (struct LogProcessData ** )lplpstartoflist;

            while (*ppLogProcessdata) {
               *ppLogProcessdata = ESL_AllocMem(sizeof(struct LogProcessData));
               if (!*ppLogProcessdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLogProcessdata,
                                             sizeof(struct LogProcessData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppLogProcessdata=&((*ppLogProcessdata)->pnext);
            }
         }
         break;
      case OT_MON_LOGDATABASE      :
         {
            struct LogDBData ** ppLogDBdata =
                                      (struct LogDBData ** )lplpstartoflist;

            while (*ppLogDBdata) {
               *ppLogDBdata = ESL_AllocMem(sizeof(struct LogDBData));
               if (!*ppLogDBdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLogDBdata,
                                             sizeof(struct LogDBData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppLogDBdata=&((*ppLogDBdata)->pnext);
            }
         }
         break;
      case OT_MON_TRANSACTION      :
         {
            struct LogTransactData ** ppLogTransactdata =
                                      (struct LogTransactData ** )lplpstartoflist;

            while (*ppLogTransactdata) {
               *ppLogTransactdata = ESL_AllocMem(sizeof(struct LogTransactData));
               if (!*ppLogTransactdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLogTransactdata,
                                             sizeof(struct LogTransactData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppLogTransactdata=&((*ppLogTransactdata)->pnext);
            }
         }
         break;
      case OT_PROFILE      :
         {
            struct ProfileData ** ppProfiledata =
                                      (struct ProfileData ** )lplpstartoflist;

            while (*ppProfiledata) {
               *ppProfiledata = ESL_AllocMem(sizeof(struct ProfileData));
               if (!*ppProfiledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppProfiledata,
                                             sizeof(struct ProfileData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppProfiledata=&((*ppProfiledata)->pnext);
            }
         }
         break;
      case OT_USER      :
         {
            struct UserData ** ppuserdata =
                                      (struct UserData ** )lplpstartoflist;

            while (*ppuserdata) {
               *ppuserdata = ESL_AllocMem(sizeof(struct UserData));
               if (!*ppuserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppuserdata,
                                             sizeof(struct UserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppuserdata=&((*ppuserdata)->pnext);
            }
         }
         break;
      case OT_GROUP     :
         {
            struct GroupData ** ppGroupdata =
                                      (struct GroupData ** )lplpstartoflist;
            int itype[]={OT_GROUPUSER };

            while (*ppGroupdata) {
               *ppGroupdata = ESL_AllocMem(sizeof(struct GroupData));
               if (!*ppGroupdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppGroupdata,
                                             sizeof(struct GroupData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_GROUPUSER:
                         pptemp = (void **)&((*ppGroupdata)->lpGroupUserData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppGroupdata=&((*ppGroupdata)->pnext);
            }
         }
         break;
      case OT_GROUPUSER :
         {
            struct GroupUserData ** ppuserdata =
                                        (struct GroupUserData ** )lplpstartoflist;

            while (*ppuserdata) {
               *ppuserdata = ESL_AllocMem(sizeof(struct GroupUserData));
               if (!*ppuserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppuserdata,
                                             sizeof(struct GroupUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppuserdata=&((*ppuserdata)->pnext);
            }
         }
         break;
      case OT_ROLE      :
         {
            struct RoleData ** ppRoledata =
                                      (struct RoleData ** )lplpstartoflist;
            int itype[]={OT_ROLEGRANT_USER };

            while (*ppRoledata) {
               *ppRoledata = ESL_AllocMem(sizeof(struct RoleData));
               if (!*ppRoledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRoledata,
                                             sizeof(struct RoleData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_ROLEGRANT_USER:
                         pptemp = (void **)&((*ppRoledata)->lpRoleGranteeData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppRoledata=&((*ppRoledata)->pnext);
            }
         }
         break;
      case OT_ROLEGRANT_USER :
         {
            struct RoleGranteeData ** ppRoleGranteedata =
                                        (struct RoleGranteeData ** )lplpstartoflist;

            while (*ppRoleGranteedata) {
               *ppRoleGranteedata = ESL_AllocMem(sizeof(struct RoleGranteeData));
               if (!*ppRoleGranteedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRoleGranteedata,
                                             sizeof(struct RoleGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRoleGranteedata=&((*ppRoleGranteedata)->pnext);
            }
         }
         break;
      case OT_LOCATION  :
         {
            struct LocationData ** ppLocationdata =
                                      (struct LocationData ** )lplpstartoflist;

            while (*ppLocationdata) {
               *ppLocationdata = ESL_AllocMem(sizeof(struct LocationData));
               if (!*ppLocationdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppLocationdata,
                                             sizeof(struct LocationData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppLocationdata=&((*ppLocationdata)->pnext);
            }
         }
         break;
      case OT_TABLE     :
         {
            struct TableData ** ppTabledata =
                                      (struct TableData ** )lplpstartoflist;
            int itype[]={OT_INTEGRITY,
                         OT_COLUMN,
                         OT_RULE,
                         OT_INDEX,
                         OT_TABLELOCATION };

            while (*ppTabledata) {
               *ppTabledata = ESL_AllocMem(sizeof(struct TableData));
               if (!*ppTabledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppTabledata,
                                             sizeof(struct TableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_INTEGRITY:
                         pptemp = (void **)&((*ppTabledata)->lpIntegrityData);break;
                      case OT_COLUMN:
                         pptemp = (void **)&((*ppTabledata)->lpColumnData)   ;break;
                      case OT_RULE:
                         pptemp = (void **)&((*ppTabledata)->lpRuleData)     ;break;
                      case OT_INDEX:
                         pptemp = (void **)&((*ppTabledata)->lpIndexData)    ;break;
                      case OT_TABLELOCATION:
                         pptemp =(void *)&((*ppTabledata)->lpTableLocationData);break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppTabledata=&((*ppTabledata)->pnext);
            }
         }
         break;
      case OT_VIEW      :
         {
            struct ViewData ** ppViewdata =
                                      (struct ViewData ** )lplpstartoflist;
            int itype[]={OT_VIEWTABLE,
                         OT_VIEWCOLUMN };

            while (*ppViewdata) {
               *ppViewdata = ESL_AllocMem(sizeof(struct ViewData));
               if (!*ppViewdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppViewdata,
                                             sizeof(struct ViewData));
               if (ires!=RES_SUCCESS)
                  return ires;
               for (i=0;i<sizeof(itype)/sizeof(int);i++) {
                   switch (itype[i]){
                      case OT_VIEWTABLE:
                         pptemp = (void **)&((*ppViewdata)->lpViewTableData);break;
                      case OT_VIEWCOLUMN:
                         pptemp = (void **)&((*ppViewdata)->lpColumnData)   ;break;
                      default:
                         return RES_ERR;
                  }
                  ires1= DOMReadList(fident, hnodestruct, itype[i], pptemp);
                  if (ires1!=RES_SUCCESS)
                     ires=ires1;
               }
               ppViewdata=&((*ppViewdata)->pnext);
            }
         }
         break;
      case OT_INTEGRITY :
         {
            struct IntegrityData ** ppIntegritydata =
                                        (struct IntegrityData ** )lplpstartoflist;

            while (*ppIntegritydata) {
               *ppIntegritydata = ESL_AllocMem(sizeof(struct IntegrityData));
               if (!*ppIntegritydata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIntegritydata,
                                             sizeof(struct IntegrityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIntegritydata=&((*ppIntegritydata)->pnext);
            }
         }
         break;
      case OT_COLUMN :
      case OT_VIEWCOLUMN :
         {
            struct ColumnData ** ppColumndata =
                                      (struct ColumnData ** )lplpstartoflist;

            while (*ppColumndata) {
               *ppColumndata = ESL_AllocMem(sizeof(struct ColumnData));
               if (!*ppColumndata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppColumndata,
                                             sizeof(struct ColumnData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppColumndata=&((*ppColumndata)->pnext);
            }
         }
         break;
      case OT_RULE   :
         {
            struct RuleData ** ppRuledata =
                                      (struct RuleData ** )lplpstartoflist;

            while (*ppRuledata) {
               *ppRuledata = ESL_AllocMem(sizeof(struct RuleData));
               if (!*ppRuledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRuledata,
                                             sizeof(struct RuleData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRuledata=&((*ppRuledata)->pnext);
            }
         }
         break;
      case OT_INDEX                      :
         {
            struct IndexData ** ppIndexdata =
                                        (struct IndexData ** )lplpstartoflist;

            while (*ppIndexdata) {
               *ppIndexdata = ESL_AllocMem(sizeof(struct IndexData));
               if (!*ppIndexdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppIndexdata,
                                             sizeof(struct IndexData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppIndexdata=&((*ppIndexdata)->pnext);
            }
            return ires;
            break;
         }
         break;
      case OT_TABLELOCATION              :
         {
            struct LocationData ** ppTableLocationdata =
                                   (struct LocationData ** )lplpstartoflist;

            while (*ppTableLocationdata) {
               *ppTableLocationdata=ESL_AllocMem(sizeof(struct LocationData));
               if (!*ppTableLocationdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppTableLocationdata,
                                             sizeof(struct LocationData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppTableLocationdata=&((*ppTableLocationdata)->pnext);
            }
            return ires;
            break;
         }
         break;
      case OT_PROCEDURE                  :
         {
            struct ProcedureData ** ppProceduredata =
                                   (struct ProcedureData ** )lplpstartoflist;

            while (*ppProceduredata) {
               *ppProceduredata = ESL_AllocMem(sizeof(struct ProcedureData));
               if (!*ppProceduredata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppProceduredata,
                                             sizeof(struct ProcedureData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppProceduredata=&((*ppProceduredata)->pnext);
            }
         }
         break;
      case OT_SEQUENCE:
         {
            struct SequenceData ** ppSequencedata =
                                   (struct SequenceData ** )lplpstartoflist;

            while (*ppSequencedata) {
               *ppSequencedata = ESL_AllocMem(sizeof(struct SequenceData));
               if (!*ppSequencedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppSequencedata,
                                             sizeof(struct SequenceData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppSequencedata=&((*ppSequencedata)->pnext);
            }
         }
         break;
      case OT_SCHEMAUSER                 :
         {
            struct SchemaUserData ** ppSchemaUserdata =
                                   (struct SchemaUserData ** )lplpstartoflist;

            while (*ppSchemaUserdata) {
               *ppSchemaUserdata = ESL_AllocMem(sizeof(struct SchemaUserData));
               if (!*ppSchemaUserdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppSchemaUserdata,
                                             sizeof(struct SchemaUserData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppSchemaUserdata=&((*ppSchemaUserdata)->pnext);
            }
         }
         break;
      case OT_SYNONYM                    :
         {
            struct SynonymData ** ppSynonymdata =
                                   (struct SynonymData ** )lplpstartoflist;

            while (*ppSynonymdata) {
               *ppSynonymdata = ESL_AllocMem(sizeof(struct SynonymData));
               if (!*ppSynonymdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppSynonymdata,
                                             sizeof(struct SynonymData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppSynonymdata=&((*ppSynonymdata)->pnext);
            }
         }
         break;
      case OT_DBEVENT                    :
         {
            struct DBEventData ** ppDBEventdata =
                                   (struct DBEventData ** )lplpstartoflist;

            while (*ppDBEventdata) {
               *ppDBEventdata = ESL_AllocMem(sizeof(struct DBEventData));
               if (!*ppDBEventdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppDBEventdata,
                                             sizeof(struct DBEventData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppDBEventdata=&((*ppDBEventdata)->pnext);
            }
         }
         break;
      case OT_REPLIC_CONNECTION :
         {
            struct ReplicConnectionData ** ppReplicConnectiondata =
                             (struct ReplicConnectionData ** )lplpstartoflist;

            while (*ppReplicConnectiondata) {
               *ppReplicConnectiondata = ESL_AllocMem(sizeof(struct ReplicConnectionData));
               if (!*ppReplicConnectiondata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppReplicConnectiondata,
                                         sizeof(struct ReplicConnectionData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppReplicConnectiondata=&((*ppReplicConnectiondata)->pnext);
            }
         }
         break;
      case OT_REPLIC_REGTABLE :
         {
            struct ReplicRegTableData ** ppReplicRegTabledata =
                              (struct ReplicRegTableData ** )lplpstartoflist;

            while (*ppReplicRegTabledata) {
               *ppReplicRegTabledata = ESL_AllocMem(sizeof(struct ReplicRegTableData));
               if (!*ppReplicRegTabledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppReplicRegTabledata,
                                           sizeof(struct ReplicRegTableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppReplicRegTabledata=&((*ppReplicRegTabledata)->pnext);
            }
         }
         break;
      case OT_REPLIC_MAILUSER :
         {
            struct ReplicMailErrData ** ppReplicMailErrdata =
                                (struct ReplicMailErrData ** )lplpstartoflist;

            while (*ppReplicMailErrdata) {
               *ppReplicMailErrdata = ESL_AllocMem(sizeof(struct ReplicMailErrData));
               if (!*ppReplicMailErrdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppReplicMailErrdata,
                                            sizeof(struct ReplicMailErrData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppReplicMailErrdata=&((*ppReplicMailErrdata)->pnext);
            }
         }
         break;
      case OTLL_REPLIC_CDDS :
         {
            struct RawReplicCDDSData ** ppRawReplicCDDSdata =
                                   (struct RawReplicCDDSData ** )lplpstartoflist;

            while (*ppRawReplicCDDSdata) {
               *ppRawReplicCDDSdata = ESL_AllocMem(sizeof(struct RawReplicCDDSData));
               if (!*ppRawReplicCDDSdata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawReplicCDDSdata,
                                             sizeof(struct RawReplicCDDSData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawReplicCDDSdata=&((*ppRawReplicCDDSdata)->pnext);
            }
         }
         break;
      case OT_VIEWTABLE :
         {
            struct ViewTableData ** ppViewTabledata =
                                   (struct ViewTableData ** )lplpstartoflist;

            while (*ppViewTabledata) {
               *ppViewTabledata = ESL_AllocMem(sizeof(struct ViewTableData));
               if (!*ppViewTabledata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppViewTabledata,
                                             sizeof(struct ViewTableData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppViewTabledata=&((*ppViewTabledata)->pnext);
            }
         }
         break;
      case OTLL_DBGRANTEE :
         {
            struct RawDBGranteeData ** ppRawDBGranteedata =
                                   (struct RawDBGranteeData ** )lplpstartoflist;

            while (*ppRawDBGranteedata) {
               *ppRawDBGranteedata = ESL_AllocMem(sizeof(struct RawDBGranteeData));
               if (!*ppRawDBGranteedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawDBGranteedata,
                                             sizeof(struct RawDBGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawDBGranteedata=&((*ppRawDBGranteedata)->pnext);
            }
         }
         break;
      case OTLL_DBSECALARM :
         {
            struct RawSecurityData ** ppRawDBSecuritydata =
                                   (struct RawSecurityData ** )lplpstartoflist;

            while (*ppRawDBSecuritydata) {
               *ppRawDBSecuritydata = ESL_AllocMem(sizeof(struct RawSecurityData));
               if (!*ppRawDBSecuritydata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawDBSecuritydata,
                                             sizeof(struct RawSecurityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawDBSecuritydata=&((*ppRawDBSecuritydata)->pnext);
            }
         }
         break;
      case OTLL_GRANTEE :
         {
            struct RawGranteeData ** ppRawGranteedata =
                                   (struct RawGranteeData ** )lplpstartoflist;

            while (*ppRawGranteedata) {
               *ppRawGranteedata = ESL_AllocMem(sizeof(struct RawGranteeData));
               if (!*ppRawGranteedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawGranteedata,
                                             sizeof(struct RawGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawGranteedata=&((*ppRawGranteedata)->pnext);
            }
         }
         break;
      case OTLL_OIDTDBGRANTEE :
         {
            struct RawDBGranteeData ** ppRawDBGranteedata =
                                   (struct RawDBGranteeData ** )lplpstartoflist;

            while (*ppRawDBGranteedata) {
               *ppRawDBGranteedata = ESL_AllocMem(sizeof(struct RawDBGranteeData));
               if (!*ppRawDBGranteedata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawDBGranteedata,
                                             sizeof(struct RawDBGranteeData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawDBGranteedata=&((*ppRawDBGranteedata)->pnext);
            }
         }
         break;
      case OTLL_SECURITYALARM :
         {
            struct RawSecurityData ** ppRawSecuritydata =
                                   (struct RawSecurityData ** )lplpstartoflist;

            while (*ppRawSecuritydata) {
               *ppRawSecuritydata = ESL_AllocMem(sizeof(struct RawSecurityData));
               if (!*ppRawSecuritydata)
                  return RES_ERR;
               ires = DBAReadData(fident, *ppRawSecuritydata,
                                             sizeof(struct RawSecurityData));
               if (ires!=RES_SUCCESS)
                  return ires;
               ppRawSecuritydata=&((*ppRawSecuritydata)->pnext);
            }
         }
         break;
      default:
         {
            char toto[40];
            // type of error1 %d 
            wsprintf(toto, ResourceString ((UINT)IDS_E_TYPEOF_ERROR1),iobjecttype);
            TS_MessageBox( TS_GetFocus(), toto, NULL, MB_OK | MB_TASKMODAL);
            return myerror(ERR_OBJECTNOEXISTS);
         }
         break;
   }
   return ires;
}

int DOMSaveCache(fident)
FILEIDENT fident;
{
   int i,ires;

   ires = DBAAppend4Save(fident, &imaxnodes, sizeof(imaxnodes));
   if (ires!=RES_SUCCESS)
      return ires;

   ires = DBAAppend4Save(fident, &DOMUpdver, sizeof(DOMUpdver));
   if (ires!=RES_SUCCESS)
      return ires;

   // save nodes list
   ires = DBAAppend4Save(fident, &lpMonNodeData, sizeof(lpMonNodeData));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAAppend4Save(fident, &nbMonNodes, sizeof(nbMonNodes));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAAppend4Save(fident, &MonNodesRefreshParms, sizeof(MonNodesRefreshParms));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAAppend4Save(fident, &monnodeserrcode, sizeof(monnodeserrcode));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DOMSaveList(fident,-1, OT_NODE, (void *) lpMonNodeData);   
   if (ires!=RES_SUCCESS)
      return ires;

   for (i=0;i<imaxnodes;i++)  {
      ires = DOMSaveList(fident,i, OT_VIRTNODE, (void *) NULL);   
      if (ires!=RES_SUCCESS)
         return ires;
   }

   return RES_SUCCESS;
}


int DOMLoadCache(fident)
FILEIDENT fident;
{
   int i,ires,itemp;

   ires = DBAReadData(fident, &itemp, sizeof(itemp));
   if (ires!=RES_SUCCESS)
      return ires;

   ires = DBAReadData(fident, &DOMUpdver, sizeof(DOMUpdver));
   if (ires!=RES_SUCCESS)
      return ires;

   imaxnodes=itemp;

   // read nodes list
   ires = DBAReadData(fident, &lpMonNodeData, sizeof(lpMonNodeData));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAReadData(fident, &nbMonNodes, sizeof(nbMonNodes));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAReadData(fident, &MonNodesRefreshParms, sizeof(MonNodesRefreshParms));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DBAReadData(fident, &monnodeserrcode, sizeof(monnodeserrcode));
   if (ires!=RES_SUCCESS)
      return ires;
   ires = DOMReadList(fident,-1, OT_NODE, (void **) &lpMonNodeData);   
   if (ires!=RES_SUCCESS)
      return ires;

   for (i=0;i<imaxnodes;i++)  {
      ires = DOMReadList(fident,i, OT_VIRTNODE, (void **) NULL);   
      if (ires!=RES_SUCCESS)
         return ires;
   }
   return RES_SUCCESS;
}

