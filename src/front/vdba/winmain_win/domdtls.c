/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres Visual DBA
**
**    Source : domdtls.c
**    Miscellaneous tools for cache
**
**  History after 24-Mar-2003:
**
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
********************************************************************/

#include "dba.h"
#include "domdata.h"
#include "domdloca.h"
#include "dbaginfo.h"
#include "error.h"

/* all GetNext.. functions are supposed to return a structure filled with */
/* zeroes if the end of the list is reached. Uses the fact that           */
/* ESL_AllocMem is specified to fill the allocated area with zeroes       */

struct ReplicConnectionData * GetNextReplicConnection(pReplicConnectiondata)
struct ReplicConnectionData * pReplicConnectiondata;
{
   if (!pReplicConnectiondata) {
      myerror(ERR_LIST);
      return pReplicConnectiondata;
   }
   if (! pReplicConnectiondata->pnext) {
      pReplicConnectiondata->pnext=ESL_AllocMem(sizeof(struct ReplicConnectionData));
      if (!pReplicConnectiondata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pReplicConnectiondata->pnext;
}

struct ReplicMailErrData * GetNextReplicMailErr(pReplicMailErrdata)
struct ReplicMailErrData * pReplicMailErrdata;
{
   if (!pReplicMailErrdata) {
      myerror(ERR_LIST);
      return pReplicMailErrdata;
   }
   if (! pReplicMailErrdata->pnext) {
      pReplicMailErrdata->pnext=ESL_AllocMem(sizeof(struct ReplicMailErrData));
      if (!pReplicMailErrdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pReplicMailErrdata->pnext;
}

struct RawReplicCDDSData * GetNextRawReplicCDDS(pRawReplicCDDSdata)
struct RawReplicCDDSData * pRawReplicCDDSdata;
{
   if (!pRawReplicCDDSdata) {
      myerror(ERR_LIST);
      return pRawReplicCDDSdata;
   }
   if (! pRawReplicCDDSdata->pnext) {
      pRawReplicCDDSdata->pnext=ESL_AllocMem(sizeof(struct RawReplicCDDSData));
      if (!pRawReplicCDDSdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pRawReplicCDDSdata->pnext;
}

struct ReplicRegTableData * GetNextReplicRegTable(pReplicRegTabledata)
struct ReplicRegTableData * pReplicRegTabledata;
{
   if (!pReplicRegTabledata) {
      myerror(ERR_LIST);
      return pReplicRegTabledata;
   }
   if (! pReplicRegTabledata->pnext) {
      pReplicRegTabledata->pnext=ESL_AllocMem(sizeof(struct ReplicRegTableData));
      if (!pReplicRegTabledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pReplicRegTabledata->pnext;
}

struct ReplicServerData * GetNextReplicServer(pReplicServerdata)
struct ReplicServerData * pReplicServerdata;
{
   if (!pReplicServerdata) {
      myerror(ERR_LIST);
      return pReplicServerdata;
   }
   if (! pReplicServerdata->pnext) {
      pReplicServerdata->pnext=ESL_AllocMem(sizeof(struct ReplicServerData));
      if (!pReplicServerdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pReplicServerdata->pnext;
}

struct IntegrityData * GetNextIntegrity(pintegritydata)
struct IntegrityData * pintegritydata;
{
   if (!pintegritydata) {
      myerror(ERR_LIST);
      return pintegritydata;
   }
   if (! pintegritydata->pnext) {
      pintegritydata->pnext=ESL_AllocMem(sizeof(struct IntegrityData));
      if (!pintegritydata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pintegritydata->pnext;
}

struct ColumnData * GetNextColumn(pColumndata)
struct ColumnData * pColumndata;
{
   if (!pColumndata) {
      myerror(ERR_LIST);
      return pColumndata;
   }
   if (! pColumndata->pnext) {
      pColumndata->pnext=ESL_AllocMem(sizeof(struct ColumnData));
      if (!pColumndata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pColumndata->pnext;
}

struct RuleData * GetNextRule(pruledata)
struct RuleData * pruledata;
{
   if (!pruledata) {
      myerror(ERR_LIST);
      return pruledata;
   }
   if (! pruledata->pnext) {
      pruledata->pnext=ESL_AllocMem(sizeof(struct RuleData));
      if (!pruledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pruledata->pnext;
}

struct SimpleLeafObjectData * GetNextSimpleLeafObject (pSimpleLeafObjectdata)
struct SimpleLeafObjectData * pSimpleLeafObjectdata;
{
   if (!pSimpleLeafObjectdata) {
      myerror(ERR_LIST);
      return pSimpleLeafObjectdata;
   }
   if (! pSimpleLeafObjectdata->pnext) {
      pSimpleLeafObjectdata->pnext=ESL_AllocMem(sizeof(struct SimpleLeafObjectData));
      if (!pSimpleLeafObjectdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pSimpleLeafObjectdata->pnext;
}
struct IceObjectWithRolesAndDBData * GetNextObjWithRoleAndDb (pIceObjdata)
struct IceObjectWithRolesAndDBData * pIceObjdata;
{
   if (!pIceObjdata) {
      myerror(ERR_LIST);
      return pIceObjdata;
   }
   if (! pIceObjdata->pnext) {
      pIceObjdata->pnext=ESL_AllocMem(sizeof(struct IceObjectWithRolesAndDBData));
      if (!pIceObjdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceObjdata->pnext;
}
struct IceObjectWithRolesAndUsersData * GetNextObjWithRoleAndUsr (pIceObjdata)
struct IceObjectWithRolesAndUsersData * pIceObjdata;
{
   if (!pIceObjdata) {
      myerror(ERR_LIST);
      return pIceObjdata;
   }
   if (! pIceObjdata->pnext) {
      pIceObjdata->pnext=ESL_AllocMem(sizeof(struct IceObjectWithRolesAndUsersData));
      if (!pIceObjdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceObjdata->pnext;
}
struct IceBusinessUnitsData * GetNextIceBusinessUnit(pIceBusinessUnitsdata)
struct IceBusinessUnitsData *pIceBusinessUnitsdata;
{
   if (!pIceBusinessUnitsdata) {
      myerror(ERR_LIST);
      return pIceBusinessUnitsdata;
   }
   if (! pIceBusinessUnitsdata->pnext) {
      pIceBusinessUnitsdata->pnext=ESL_AllocMem(sizeof(struct IceBusinessUnitsData));
      if (!pIceBusinessUnitsdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceBusinessUnitsdata->pnext;

}

struct SecurityData * GetNextSecurity(psecuritydata)
struct SecurityData * psecuritydata;
{
   if (!psecuritydata) {
      myerror(ERR_LIST);
      return psecuritydata;
   }
   if (! psecuritydata->pnext) {
      psecuritydata->pnext=ESL_AllocMem(sizeof(struct SecurityData));
      if (!psecuritydata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return psecuritydata->pnext;
}

struct RawSecurityData * GetNextRawSecurity(prawsecuritydata)
struct RawSecurityData * prawsecuritydata;
{
   if (!prawsecuritydata) {
      myerror(ERR_LIST);
      return prawsecuritydata;
   }
   if (! prawsecuritydata->pnext) {
      prawsecuritydata->pnext=ESL_AllocMem(sizeof(struct RawSecurityData));
      if (!prawsecuritydata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return prawsecuritydata->pnext;
}

struct IndexData * GetNextIndex(pindexdata)
struct IndexData * pindexdata;
{
   if (!pindexdata) {
      myerror(ERR_LIST);
      return pindexdata;
   }
   if (! pindexdata->pnext) {
      pindexdata->pnext=ESL_AllocMem(sizeof(struct IndexData));
      if (!pindexdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pindexdata->pnext;
}

struct GranteeData * GetNextGrantee(pgranteedata)
struct GranteeData * pgranteedata;
{
   if (!pgranteedata) {
      myerror(ERR_LIST);
      return pgranteedata;
   }
   if (! pgranteedata->pnext) {
      pgranteedata->pnext=ESL_AllocMem(sizeof(struct GranteeData));
      if (!pgranteedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pgranteedata->pnext;
}

struct RawGranteeData * GetNextRawGrantee(prawGranteedata)
struct RawGranteeData * prawGranteedata;
{
   if (!prawGranteedata) {
      myerror(ERR_LIST);
      return prawGranteedata;
   }
   if (! prawGranteedata->pnext) {
      prawGranteedata->pnext=ESL_AllocMem(sizeof(struct RawGranteeData));
      if (!prawGranteedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return prawGranteedata->pnext;
}

struct RawDBGranteeData * GetNextRawDBGrantee(prawDBGranteedata)
struct RawDBGranteeData * prawDBGranteedata;
{
   if (!prawDBGranteedata) {
      myerror(ERR_LIST);
      return prawDBGranteedata;
   }
   if (! prawDBGranteedata->pnext) {
      prawDBGranteedata->pnext=ESL_AllocMem(sizeof(struct RawDBGranteeData));
      if (!prawDBGranteedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return prawDBGranteedata->pnext;
}

struct DBEventData * GetNextDBEvent(pdbeventdata)
struct DBEventData * pdbeventdata;
{
   if (!pdbeventdata) {
      myerror(ERR_LIST);
      return pdbeventdata;
   }
   if (! pdbeventdata->pnext) {
      pdbeventdata->pnext=ESL_AllocMem(sizeof(struct DBEventData));
      if (!pdbeventdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pdbeventdata->pnext;
}

struct ProcedureData * GetNextProcedure(pproceduredata)
struct ProcedureData * pproceduredata;
{
   if (!pproceduredata) {
      myerror(ERR_LIST);
      return pproceduredata;
   }
   if (! pproceduredata->pnext) {
      pproceduredata->pnext=ESL_AllocMem(sizeof(struct ProcedureData));
      if (!pproceduredata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pproceduredata->pnext;
}

struct SequenceData * GetNextSequence(pSequencedata)
struct SequenceData * pSequencedata;
{
   if (!pSequencedata) {
      myerror(ERR_LIST);
      return pSequencedata;
   }
   if (! pSequencedata->pnext) {
      pSequencedata->pnext=ESL_AllocMem(sizeof(struct SequenceData));
      if (!pSequencedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pSequencedata->pnext;
}
struct SynonymData * GetNextSynonym(psynonymdata)
struct SynonymData * psynonymdata;
{
   if (!psynonymdata) {
      myerror(ERR_LIST);
      return psynonymdata;
   }
   if (! psynonymdata->pnext) {
      psynonymdata->pnext=ESL_AllocMem(sizeof(struct SynonymData));
      if (!psynonymdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return psynonymdata->pnext;
}

struct SchemaUserData * GetNextSchemaUser(pschemauserdata)
struct SchemaUserData * pschemauserdata;
{
   if (!pschemauserdata) {
      myerror(ERR_LIST);
      return pschemauserdata;
   }
   if (! pschemauserdata->pnext) {
      pschemauserdata->pnext=ESL_AllocMem(sizeof(struct SchemaUserData));
      if (!pschemauserdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pschemauserdata->pnext;
}

struct TableData * GetNextTable(pTabledata)
struct TableData * pTabledata;
{
   if (!pTabledata) {
      myerror(ERR_LIST);
      return pTabledata;
   }
   if (! pTabledata->pnext) {
      pTabledata->pnext=ESL_AllocMem(sizeof(struct TableData));
      if (!pTabledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pTabledata->pnext;
}

struct ViewData * GetNextView(pViewdata)
struct ViewData * pViewdata;
{
   if (!pViewdata) {
      myerror(ERR_LIST);
      return pViewdata;
   }
   if (! pViewdata->pnext) {
      pViewdata->pnext=ESL_AllocMem(sizeof(struct ViewData));
      if (!pViewdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pViewdata->pnext;
}

struct ViewTableData * GetNextViewTable(pViewTabledata)
struct ViewTableData * pViewTabledata;
{
   if (!pViewTabledata) {
      myerror(ERR_LIST);
      return pViewTabledata;
   }
   if (! pViewTabledata->pnext) {
      pViewTabledata->pnext=ESL_AllocMem(sizeof(struct ViewTableData));
      if (!pViewTabledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pViewTabledata->pnext;
}

struct ProfileData * GetNextProfile(pProfiledata)
struct ProfileData * pProfiledata;
{
   if (!pProfiledata) {
      myerror(ERR_LIST);
      return pProfiledata;
   }
   if (! pProfiledata->pnext) {
      pProfiledata->pnext=ESL_AllocMem(sizeof(struct ProfileData));
      if (!pProfiledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pProfiledata->pnext;
}

struct UserData * GetNextUser(pUserdata)
struct UserData * pUserdata;
{
   if (!pUserdata) {
      myerror(ERR_LIST);
      return pUserdata;
   }
   if (! pUserdata->pnext) {
      pUserdata->pnext=ESL_AllocMem(sizeof(struct UserData));
      if (!pUserdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pUserdata->pnext;
}

struct GroupData * GetNextGroup(pGroupdata)
struct GroupData * pGroupdata;
{
   if (!pGroupdata) {
      myerror(ERR_LIST);
      return pGroupdata;
   }
   if (! pGroupdata->pnext) {
      pGroupdata->pnext=ESL_AllocMem(sizeof(struct GroupData));
      if (!pGroupdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pGroupdata->pnext;
}

struct GroupUserData * GetNextGroupUser(pGroupUserdata)
struct GroupUserData * pGroupUserdata;
{
   if (!pGroupUserdata) {
      myerror(ERR_LIST);
      return pGroupUserdata;
   }
   if (! pGroupUserdata->pnext) {
      pGroupUserdata->pnext=ESL_AllocMem(sizeof(struct GroupUserData));
      if (!pGroupUserdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pGroupUserdata->pnext;
}

struct RoleData * GetNextRole(pRoledata)
struct RoleData * pRoledata;
{
   if (!pRoledata) {
      myerror(ERR_LIST);
      return pRoledata;
   }
   if (! pRoledata->pnext) {
      pRoledata->pnext=ESL_AllocMem(sizeof(struct RoleData));
      if (!pRoledata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pRoledata->pnext;
}

struct RoleGranteeData * GetNextRoleGrantee(pRoleGranteedata)
struct RoleGranteeData * pRoleGranteedata;
{
   if (!pRoleGranteedata) {
      myerror(ERR_LIST);
      return pRoleGranteedata;
   }
   if (! pRoleGranteedata->pnext) {
      pRoleGranteedata->pnext=ESL_AllocMem(sizeof(struct RoleGranteeData));
      if (!pRoleGranteedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pRoleGranteedata->pnext;
}

struct LocationData * GetNextLocation(pLocationdata)
struct LocationData * pLocationdata;
{
   if (!pLocationdata) {
      myerror(ERR_LIST);
      return pLocationdata;
   }
   if (! pLocationdata->pnext) {
      pLocationdata->pnext=ESL_AllocMem(sizeof(struct LocationData));
      if (!pLocationdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLocationdata->pnext;
}

struct DBData * GetNextDB(pDBdata)
struct DBData * pDBdata;
{
   if (!pDBdata) {
      myerror(ERR_LIST);
      return pDBdata;
   }
   if (! pDBdata->pnext) {
      pDBdata->pnext=ESL_AllocMem(sizeof(struct DBData));
      if (!pDBdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pDBdata->pnext;
}


struct NodeServerClassData * GetNextServerClass(pServerClassdata)
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

struct NodeLoginData * GetNextNodeLogin(pNodeLogindata)
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

struct NodeConnectionData * GetNextNodeConnection(pNodeConnectiondata)
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

struct NodeAttributeData  * GetNextNodeAttribute(pNodeAttributeData)
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

struct MonNodeData * GetNextMonNode(pMonNodedata)
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


struct WindowData * GetNextNodeWindow(pWindowdata)
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

// MONITORING

struct ServerData * GetNextServer(pServerdata)
struct ServerData * pServerdata;
{
   if (!pServerdata) {
      myerror(ERR_LIST);
      return pServerdata;
   }
   if (! pServerdata->pnext) {
      pServerdata->pnext=ESL_AllocMem(sizeof(struct ServerData));
      if (!pServerdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pServerdata->pnext;
}

struct MonReplServerData * GetNextMonReplServer(pMonReplServerdata)
struct MonReplServerData * pMonReplServerdata;
{
   if (!pMonReplServerdata) {
      myerror(ERR_LIST);
      return pMonReplServerdata;
   }
   if (! pMonReplServerdata->pnext) {
      pMonReplServerdata->pnext=ESL_AllocMem(sizeof(struct MonReplServerData));
      if (!pMonReplServerdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pMonReplServerdata->pnext;
}

struct SessionData * GetNextSession(pSessiondata)
struct SessionData * pSessiondata;
{
   if (!pSessiondata) {
      myerror(ERR_LIST);
      return pSessiondata;
   }
   if (! pSessiondata->pnext) {
      pSessiondata->pnext=ESL_AllocMem(sizeof(struct SessionData));
      if (!pSessiondata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pSessiondata->pnext;
}

struct IceConnUserData * GetNextIceConnUser(pIceConnectUserdata)
struct IceConnUserData * pIceConnectUserdata;
{
   if (!pIceConnectUserdata) {
      myerror(ERR_LIST);
      return pIceConnectUserdata;
   }
   if (! pIceConnectUserdata->pnext) {
      pIceConnectUserdata->pnext=ESL_AllocMem(sizeof(struct IceConnUserData));
      if (!pIceConnectUserdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceConnectUserdata->pnext;
}

struct IceActiveUserData * GetNextIceActiveUser(pIceActiveUserdata)
struct IceActiveUserData * pIceActiveUserdata;
{
   if (!pIceActiveUserdata) {
      myerror(ERR_LIST);
      return pIceActiveUserdata;
   }
   if (! pIceActiveUserdata->pnext) {
      pIceActiveUserdata->pnext=ESL_AllocMem(sizeof(struct IceActiveUserData));
      if (!pIceActiveUserdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceActiveUserdata->pnext;
}

struct IceTransactData * GetNextIceTransaction(pIceTransactiondata)
struct IceTransactData * pIceTransactiondata;
{
   if (!pIceTransactiondata) {
      myerror(ERR_LIST);
      return pIceTransactiondata;
   }
   if (! pIceTransactiondata->pnext) {
      pIceTransactiondata->pnext=ESL_AllocMem(sizeof(struct IceTransactData));
      if (!pIceTransactiondata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceTransactiondata->pnext;
}

struct IceCursorData * GetNextIceCursor(pIceCursordata)
struct IceCursorData * pIceCursordata;
{
   if (!pIceCursordata) {
      myerror(ERR_LIST);
      return pIceCursordata;
   }
   if (! pIceCursordata->pnext) {
      pIceCursordata->pnext=ESL_AllocMem(sizeof(struct IceCursorData));
      if (!pIceCursordata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceCursordata->pnext;
}

struct IceCacheData * GetNextIceCacheInfo(pIceCachedata)
struct IceCacheData * pIceCachedata;
{
   if (!pIceCachedata) {
      myerror(ERR_LIST);
      return pIceCachedata;
   }
   if (! pIceCachedata->pnext) {
      pIceCachedata->pnext=ESL_AllocMem(sizeof(struct IceCacheData));
      if (!pIceCachedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceCachedata->pnext;
}

struct IceDbConnectData * GetNextIceDbConnectdata(pIceDbConnectdata)
struct IceDbConnectData * pIceDbConnectdata;
{
   if (!pIceDbConnectdata) {
      myerror(ERR_LIST);
      return pIceDbConnectdata;
   }
   if (! pIceDbConnectdata->pnext) {
      pIceDbConnectdata->pnext=ESL_AllocMem(sizeof(struct IceDbConnectData));
      if (!pIceDbConnectdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pIceDbConnectdata->pnext;
}


struct LockListData * GetNextLockList(pLockListdata)
struct LockListData * pLockListdata;
{
   if (!pLockListdata) {
      myerror(ERR_LIST);
      return pLockListdata;
   }
   if (! pLockListdata->pnext) {
      pLockListdata->pnext=ESL_AllocMem(sizeof(struct LockListData));
      if (!pLockListdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLockListdata->pnext;
}

struct LockData * GetNextLock(pLockdata)
struct LockData * pLockdata;
{
   if (!pLockdata) {
      myerror(ERR_LIST);
      return pLockdata;
   }
   if (! pLockdata->pnext) {
      pLockdata->pnext=ESL_AllocMem(sizeof(struct LockData));
      if (!pLockdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLockdata->pnext;
}

struct ResourceData * GetNextResource(pResourcedata)
struct ResourceData * pResourcedata;
{
   if (!pResourcedata) {
      myerror(ERR_LIST);
      return pResourcedata;
   }
   if (! pResourcedata->pnext) {
      pResourcedata->pnext=ESL_AllocMem(sizeof(struct ResourceData));
      if (!pResourcedata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pResourcedata->pnext;
}

struct LogProcessData * GetNextLogProcess(pLogProcessdata)
struct LogProcessData * pLogProcessdata;
{
   if (!pLogProcessdata) {
      myerror(ERR_LIST);
      return pLogProcessdata;
   }
   if (! pLogProcessdata->pnext) {
      pLogProcessdata->pnext=ESL_AllocMem(sizeof(struct LogProcessData));
      if (!pLogProcessdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLogProcessdata->pnext;
}

struct LogDBData * GetNextLogDB(pLogDBdata)
struct LogDBData * pLogDBdata;
{
   if (!pLogDBdata) {
      myerror(ERR_LIST);
      return pLogDBdata;
   }
   if (! pLogDBdata->pnext) {
      pLogDBdata->pnext=ESL_AllocMem(sizeof(struct LogDBData));
      if (!pLogDBdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLogDBdata->pnext;
}

struct LogTransactData * GetNextLogTransact(pLogTransactdata)
struct LogTransactData * pLogTransactdata;
{
   if (!pLogTransactdata) {
      myerror(ERR_LIST);
      return pLogTransactdata;
   }
   if (! pLogTransactdata->pnext) {
      pLogTransactdata->pnext=ESL_AllocMem(sizeof(struct LogTransactData));
      if (!pLogTransactdata->pnext)
         myerror(ERR_ALLOCMEM);
   }
   return pLogTransactdata->pnext;
}

BOOL freeServerData(lpServerdata)
struct ServerData * lpServerdata;
{
   struct ServerData *ptemp;

   while(lpServerdata) {
      ptemp=lpServerdata->pnext;
      freeSessionData        (lpServerdata->lpServerSessionData);
      freeIceConnectUsersData(lpServerdata->lpIceConnUsersData);
      freeIceActiveUsersData (lpServerdata->lpIceActiveUsersData);
      freeIcetransactionsData(lpServerdata->lpIceTransactionsData);
      freeIceCursorsData     (lpServerdata->lpIceCursorsData);
      freeIceCacheData       (lpServerdata->lpIceCacheInfoData);
      freeIceDbConnectData   (lpServerdata->lpIceDbConnectData);
      ESL_FreeMem(lpServerdata);  
      lpServerdata = ptemp;
   }
   return TRUE;
}

BOOL freeMonReplServerData(lpMonReplServerdata)
struct MonReplServerData * lpMonReplServerdata;
{
   struct MonReplServerData *ptemp;

   while(lpMonReplServerdata) {
      ptemp=lpMonReplServerdata->pnext;
	  if (lpMonReplServerdata->MonReplServerDta.DBEhdl>=0 && lpMonReplServerdata->MonReplServerDta.LocalDBName[0]) { // LocalDBName test for unused end of list entry
		  DBEReplReplReleaseEntry(lpMonReplServerdata->MonReplServerDta.DBEhdl);
	  }
      ESL_FreeMem(lpMonReplServerdata);  
      lpMonReplServerdata = ptemp;
   }
   return TRUE;
}

BOOL freeSessionData(lpSessiondata)
struct SessionData * lpSessiondata;
{
   struct SessionData *ptemp;

   while(lpSessiondata) {
      ptemp=lpSessiondata->pnext;
      ESL_FreeMem(lpSessiondata);  
      lpSessiondata = ptemp;
   }
   return TRUE;
}

BOOL freeIceConnectUsersData(pIceConnectUsersdata)
struct IceConnUserData   *   pIceConnectUsersdata;
{
   struct IceConnUserData *ptemp;

   while(pIceConnectUsersdata) {
      ptemp=pIceConnectUsersdata->pnext;
      ESL_FreeMem(pIceConnectUsersdata);
      pIceConnectUsersdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceActiveUsersData (pIceActiveUsersdata)
struct IceActiveUserData *   pIceActiveUsersdata;
{
   struct IceActiveUserData *ptemp;

   while(pIceActiveUsersdata) {
      ptemp=pIceActiveUsersdata->pnext;
      ESL_FreeMem(pIceActiveUsersdata);
      pIceActiveUsersdata = ptemp;
   }
   return TRUE;
}

BOOL freeIcetransactionsData(pIceTransactionsdata)
struct IceTransactData *     pIceTransactionsdata;
{
   struct IceTransactData *ptemp;

   while(pIceTransactionsdata) {
      ptemp=pIceTransactionsdata->pnext;
      ESL_FreeMem(pIceTransactionsdata);
      pIceTransactionsdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceCursorsData(pIceCursorsdata)
struct IceCursorData *  pIceCursorsdata;
{
   struct IceCursorData *ptemp;

   while(pIceCursorsdata) {
      ptemp=pIceCursorsdata->pnext;
      ESL_FreeMem(pIceCursorsdata);
      pIceCursorsdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceCacheData(pIceCachedata)
struct IceCacheData * pIceCachedata;
{
   struct IceCacheData *ptemp;

   while(pIceCachedata) {
      ptemp=pIceCachedata->pnext;
      ESL_FreeMem(pIceCachedata);
      pIceCachedata = ptemp;
   }
   return TRUE;
}

BOOL freeIceDbConnectData(pIceDbConnectdata)
struct IceDbConnectData * pIceDbConnectdata;
{
   struct IceDbConnectData *ptemp;

   while(pIceDbConnectdata) {
      ptemp=pIceDbConnectdata->pnext;
      ESL_FreeMem(pIceDbConnectdata);
      pIceDbConnectdata = ptemp;
   }
   return TRUE;
}

// static ICE DOM branches

BOOL freeSimpleLeafObjectData       (pSimpleLeafObjectdata)
struct SimpleLeafObjectData * pSimpleLeafObjectdata;
{
   struct SimpleLeafObjectData *ptemp;

   while(pSimpleLeafObjectdata) {
      ptemp=pSimpleLeafObjectdata->pnext;
      ESL_FreeMem(pSimpleLeafObjectdata);
      pSimpleLeafObjectdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceObjectWithRolesAndDBData(pIceObjdata)
struct IceObjectWithRolesAndDBData * pIceObjdata;
{
   struct IceObjectWithRolesAndDBData *ptemp;

   while(pIceObjdata) {
      ptemp=pIceObjdata->pnext;
      freeSimpleLeafObjectData(pIceObjdata->lpObjectRolesData);
      freeSimpleLeafObjectData(pIceObjdata->lpObjectDbData);
      ESL_FreeMem(pIceObjdata);
      pIceObjdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceObjectWithRolesAndUsrData(pIceObjdata)
struct IceObjectWithRolesAndUsersData * pIceObjdata;
{
   struct IceObjectWithRolesAndUsersData *ptemp;

   while(pIceObjdata) {
      ptemp=pIceObjdata->pnext;
      freeSimpleLeafObjectData(pIceObjdata->lpObjectRolesData);
      freeSimpleLeafObjectData(pIceObjdata->lpObjectUsersData);
      ESL_FreeMem(pIceObjdata);
      pIceObjdata = ptemp;
   }
   return TRUE;
}

BOOL freeIceBusinessUnitsData       (pIceBusinessUnitsdata)
struct IceBusinessUnitsData *pIceBusinessUnitsdata;
{
   struct IceBusinessUnitsData *ptemp;

   while(pIceBusinessUnitsdata) {
      ptemp=pIceBusinessUnitsdata->pnext;
      freeSimpleLeafObjectData(pIceBusinessUnitsdata->lpObjectRolesData);
      freeSimpleLeafObjectData(pIceBusinessUnitsdata->lpObjectUsersData);
      freeIceObjectWithRolesAndUsrData(pIceBusinessUnitsdata->lpObjectFacetsData);
      freeIceObjectWithRolesAndUsrData(pIceBusinessUnitsdata->lpObjectPagesData);
      freeSimpleLeafObjectData(pIceBusinessUnitsdata->lpObjectLocationsData);
      ESL_FreeMem(pIceBusinessUnitsdata);
      pIceBusinessUnitsdata = ptemp;
   }
   return TRUE;
}


BOOL freeLockListData(lpLockListdata)
struct LockListData * lpLockListdata;
{
   struct LockListData *ptemp;

   while(lpLockListdata) {
      ptemp=lpLockListdata->pnext;
      freeLockData       (lpLockListdata->lpLockData);
      ESL_FreeMem(lpLockListdata);  
      lpLockListdata = ptemp;
   }
   return TRUE;
}

BOOL freeLockData(lpLockdata)
struct LockData * lpLockdata;
{
   struct LockData *ptemp;

   while(lpLockdata) {
      ptemp=lpLockdata->pnext;
      ESL_FreeMem(lpLockdata);  
      lpLockdata = ptemp;
   }
   return TRUE;
}

BOOL freeResourceData(lpResourcedata)
struct ResourceData * lpResourcedata;
{
   struct ResourceData *ptemp;

   while(lpResourcedata) {
      ptemp=lpResourcedata->pnext;
      freeLockData(lpResourcedata->lpResourceLockData);
      ESL_FreeMem(lpResourcedata);  
      lpResourcedata = ptemp;
   }
   return TRUE;
}

BOOL freeLogProcessData(lpLogProcessdata)
struct LogProcessData * lpLogProcessdata;
{
   struct LogProcessData *ptemp;

   while(lpLogProcessdata) {
      ptemp=lpLogProcessdata->pnext;
      ESL_FreeMem(lpLogProcessdata);  
      lpLogProcessdata = ptemp;
   }
   return TRUE;
}

BOOL freeLogDBData(lpLogDBdata)
struct LogDBData * lpLogDBdata;
{
   struct LogDBData *ptemp;

   while(lpLogDBdata) {
      ptemp=lpLogDBdata->pnext;
      ESL_FreeMem(lpLogDBdata);  
      lpLogDBdata = ptemp;
   }
   return TRUE;
}

BOOL freeLogTransactData(lpLogTransactdata)
struct LogTransactData * lpLogTransactdata;
{
   struct LogTransactData *ptemp;

   while(lpLogTransactdata) {
      ptemp=lpLogTransactdata->pnext;
      ESL_FreeMem(lpLogTransactdata);  
      lpLogTransactdata = ptemp;
   }
   return TRUE;
}
// end of monitoring


BOOL freeIntegritiesData(lpIntegritiesdata)
struct IntegrityData *lpIntegritiesdata;
{
   struct IntegrityData *ptemp;

   while(lpIntegritiesdata) {
      ptemp=lpIntegritiesdata->pnext;
      ESL_FreeMem(lpIntegritiesdata);  
      lpIntegritiesdata = ptemp;
   }
   return TRUE;
}

BOOL freeReplicConnectionsData(lpReplicConnectionsdata)
struct ReplicConnectionData *lpReplicConnectionsdata;
{
   struct ReplicConnectionData *ptemp;

   while(lpReplicConnectionsdata) {
      ptemp=lpReplicConnectionsdata->pnext;
      ESL_FreeMem(lpReplicConnectionsdata);  
      lpReplicConnectionsdata = ptemp;
   }
   return TRUE;
}

BOOL freeReplicMailErrsData(lpReplicMailErrsdata)
struct ReplicMailErrData *lpReplicMailErrsdata;
{
   struct ReplicMailErrData *ptemp;

   while(lpReplicMailErrsdata) {
      ptemp=lpReplicMailErrsdata->pnext;
      ESL_FreeMem(lpReplicMailErrsdata);  
      lpReplicMailErrsdata = ptemp;
   }
   return TRUE;
}

BOOL freeRawReplicCDDSData(lpRawReplicCDDSdata)
struct RawReplicCDDSData  *lpRawReplicCDDSdata;
{
   struct RawReplicCDDSData *ptemp;

   while(lpRawReplicCDDSdata) {
      ptemp=lpRawReplicCDDSdata->pnext;
      ESL_FreeMem(lpRawReplicCDDSdata);  
      lpRawReplicCDDSdata = ptemp;
   }
   return TRUE;
}

BOOL freeReplicRegTablesData(lpReplicRegTablesdata)
struct ReplicRegTableData *lpReplicRegTablesdata;
{
   struct ReplicRegTableData *ptemp;

   while(lpReplicRegTablesdata) {
      ptemp=lpReplicRegTablesdata->pnext;
      ESL_FreeMem(lpReplicRegTablesdata);  
      lpReplicRegTablesdata = ptemp;
   }
   return TRUE;
}

BOOL freeReplicServersData(lpReplicServersdata)
struct ReplicServerData *lpReplicServersdata;
{
   struct ReplicServerData *ptemp;

   while(lpReplicServersdata) {
      ptemp=lpReplicServersdata->pnext;
      ESL_FreeMem(lpReplicServersdata);  
      lpReplicServersdata = ptemp;
   }
   return TRUE;
}

BOOL freeColumnsData(lpColumnsdata)
struct ColumnData *lpColumnsdata;
{
   struct ColumnData *ptemp;

   while(lpColumnsdata) {
      ptemp=lpColumnsdata->pnext;
      ESL_FreeMem(lpColumnsdata);  
      lpColumnsdata = ptemp;
   }
   return TRUE;
}

BOOL freeRulesData(lpRulesdata)
struct RuleData *lpRulesdata;
{
   struct RuleData *ptemp;

   while(lpRulesdata) {
      ptemp=lpRulesdata->pnext;
      ESL_FreeMem(lpRulesdata);  
      lpRulesdata = ptemp;
   }
   return TRUE;
}

BOOL freeSecuritiesData(lpSecuritiesdata)
struct SecurityData *lpSecuritiesdata;
{
   struct SecurityData *ptemp;

   while(lpSecuritiesdata) {
      ptemp=lpSecuritiesdata->pnext;
      ESL_FreeMem(lpSecuritiesdata);  
      lpSecuritiesdata = ptemp;
   }
   return TRUE;
}

BOOL freeRawSecuritiesData(lpRawSecuritiesdata)
struct RawSecurityData *lpRawSecuritiesdata;
{
   struct RawSecurityData *ptemp;

   while(lpRawSecuritiesdata) {
      ptemp=lpRawSecuritiesdata->pnext;
      ESL_FreeMem(lpRawSecuritiesdata);  
      lpRawSecuritiesdata = ptemp;
   }
   return TRUE;
}

BOOL freeIndexData(lpIndexdata)
struct IndexData *lpIndexdata;
{
   struct IndexData *ptemp;

   while(lpIndexdata) {
      ptemp=lpIndexdata->pnext;
      ESL_FreeMem(lpIndexdata);  
      lpIndexdata = ptemp;
   }
   return TRUE;
}

BOOL freeGranteesData(lpGranteedata)
struct GranteeData *lpGranteedata;
{
   struct GranteeData *ptemp;

   while(lpGranteedata) {
      ptemp=lpGranteedata->pnext;
      ESL_FreeMem(lpGranteedata);  
      lpGranteedata = ptemp;
   }
   return TRUE;
}

BOOL freeRawGranteesData(lpRawGranteedata)
struct RawGranteeData *lpRawGranteedata;
{
   struct RawGranteeData *ptemp;

   while(lpRawGranteedata) {
      ptemp=lpRawGranteedata->pnext;
      ESL_FreeMem(lpRawGranteedata);  
      lpRawGranteedata = ptemp;
   }
   return TRUE;
}

BOOL freeRawDBGranteesData(lpRawDBGranteedata)
struct RawDBGranteeData *lpRawDBGranteedata;
{
   struct RawDBGranteeData *ptemp;

   while(lpRawDBGranteedata) {
      ptemp=lpRawDBGranteedata->pnext;
      ESL_FreeMem(lpRawDBGranteedata);  
      lpRawDBGranteedata = ptemp;
   }
   return TRUE;
}

BOOL freeDBEventsData(lpDBEventdata)
struct DBEventData *lpDBEventdata;
{
   struct DBEventData *ptemp;

   while(lpDBEventdata) {
      ptemp=lpDBEventdata->pnext;
      ESL_FreeMem(lpDBEventdata);  
      lpDBEventdata = ptemp;
   }
   return TRUE;
}

BOOL freeLocationsData(lpLocationData)
struct LocationData *lpLocationData;
{

   struct LocationData *ptemp;

   while(lpLocationData) {
      ptemp=lpLocationData->pnext;
      ESL_FreeMem(lpLocationData);  
      lpLocationData = ptemp;
   }
   return TRUE;
}

BOOL freeTablesData(lpTabledata)
struct TableData *lpTabledata;
{
   struct TableData *ptemp;

   while(lpTabledata) {
      ptemp=lpTabledata->pnext;
      freeIntegritiesData   (lpTabledata->lpIntegrityData);
      freeColumnsData       (lpTabledata->lpColumnData);
      freeRulesData         (lpTabledata->lpRuleData);
      freeSecuritiesData    (lpTabledata->lpSecurityData);
      freeIndexData         (lpTabledata->lpIndexData);
      freeLocationsData     (lpTabledata->lpTableLocationData);
      freeGranteesData      (lpTabledata->lpGranteeData);
      ESL_FreeMem(lpTabledata);  
      lpTabledata = ptemp;
   }
   return TRUE;
}

BOOL freeViewTablesData(lpViewTabledata)
struct ViewTableData *lpViewTabledata;
{
   struct ViewTableData *ptemp;

   while(lpViewTabledata) {
      ptemp=lpViewTabledata->pnext;
      ESL_FreeMem(lpViewTabledata);  
      lpViewTabledata = ptemp;
   }
   return TRUE;
}

BOOL freeViewsData(lpViewdata)
struct ViewData *lpViewdata;
{
   struct ViewData *ptemp;

   while(lpViewdata) {
      ptemp=lpViewdata->pnext;
      freeViewTablesData   (lpViewdata->lpViewTableData);
      freeGranteesData     (lpViewdata->lpGranteeData);
      freeColumnsData      (lpViewdata->lpColumnData);
      ESL_FreeMem(lpViewdata);  
      lpViewdata = ptemp;
   }
   return TRUE;
}

BOOL freeProceduresData(lpProcedureData)   
struct ProcedureData  * lpProcedureData;
{
   struct ProcedureData *ptemp;

   while(lpProcedureData) {
      ptemp=lpProcedureData->pnext;
      ESL_FreeMem(lpProcedureData);  
      lpProcedureData = ptemp;
   }
   return TRUE;
}

BOOL freeSequencesData(lpSequenceData)   
struct SequenceData  * lpSequenceData;
{
   struct SequenceData *ptemp;

   while(lpSequenceData) {
      ptemp=lpSequenceData->pnext;
      ESL_FreeMem(lpSequenceData);  
      lpSequenceData = ptemp;
   }
   return TRUE;
}

BOOL freeSynonymsData(lpSynonymData)   
struct SynonymData  * lpSynonymData;
{
   struct SynonymData *ptemp;

   while(lpSynonymData) {
      ptemp=lpSynonymData->pnext;
      ESL_FreeMem(lpSynonymData);  
      lpSynonymData = ptemp;
   }
   return TRUE;
}

BOOL freeSchemaUsersData(lpSchemaUserData)   
struct SchemaUserData  * lpSchemaUserData;
{
   struct SchemaUserData *ptemp;

   while(lpSchemaUserData) {
      ptemp=lpSchemaUserData->pnext;
      ESL_FreeMem(lpSchemaUserData);  
      lpSchemaUserData = ptemp;
   }
   return TRUE;
}

BOOL freeDBData(lpDBdata)
struct DBData * lpDBdata;
{
   struct DBData *ptemp;

   while(lpDBdata) {
      ptemp=lpDBdata->pnext;
      freeTablesData       (lpDBdata->lpTableData);
      freeViewsData        (lpDBdata->lpViewData);
      freeProceduresData   (lpDBdata->lpProcedureData);
      freeSequencesData    (lpDBdata->lpSequenceData);
      freeSchemaUsersData  (lpDBdata->lpSchemaUserData);
      freeSynonymsData     (lpDBdata->lpSynonymData);
      freeDBEventsData     (lpDBdata->lpDBEventData);
      freeGranteesData     (lpDBdata->lpDBGranteeData);
      freeRawGranteesData  (lpDBdata->lpRawGranteeData);
      freeRawSecuritiesData(lpDBdata->lpRawSecurityData);
      freeReplicConnectionsData(lpDBdata->lpReplicConnectionData);
      freeReplicMailErrsData   (lpDBdata->lpReplicMailErrData);
      freeRawReplicCDDSData    (lpDBdata->lpRawReplicCDDSData);
      freeReplicRegTablesData  (lpDBdata->lpReplicRegTableData);
      freeReplicServersData    (lpDBdata->lpReplicServerData);
      freeRawDBGranteesData    (lpDBdata->lpOIDTDBGranteeData);
	  freeMonReplServerData    (lpDBdata->lpMonReplServerData);
      ESL_FreeMem(lpDBdata);  
      lpDBdata = ptemp;
   }
   return TRUE;
}

BOOL freeProfilesData(lpProfileData)   
struct ProfileData  * lpProfileData;
{
   struct ProfileData *ptemp;

   while(lpProfileData) {
      ptemp=lpProfileData->pnext;
      ESL_FreeMem(lpProfileData);  
      lpProfileData = ptemp;
   }
   return TRUE;
}

BOOL freeUsersData(lpUserData)   
struct UserData  * lpUserData;
{
   struct UserData *ptemp;

   while(lpUserData) {
      ptemp=lpUserData->pnext;
      ESL_FreeMem(lpUserData);  
      lpUserData = ptemp;
   }
   return TRUE;
}

BOOL freeGroupUsersData(lpGroupUserData)   
struct GroupUserData  * lpGroupUserData;
{
   struct GroupUserData *ptemp;

   while(lpGroupUserData) {
      ptemp=lpGroupUserData->pnext;
      ESL_FreeMem(lpGroupUserData);  
      lpGroupUserData = ptemp;
   }
   return TRUE;
}

BOOL freeGroupsData(lpGroupData)
struct GroupData * lpGroupData;
{

   struct GroupData *ptemp;

   while(lpGroupData) {
      ptemp=lpGroupData->pnext;
      freeGroupUsersData(lpGroupData->lpGroupUserData);
      ESL_FreeMem(lpGroupData);  
      lpGroupData = ptemp;
   }
   return TRUE;
}

BOOL freeRoleGranteesData(lpRoleGranteeData)   
struct RoleGranteeData  * lpRoleGranteeData;
{
   struct RoleGranteeData *ptemp;

   while(lpRoleGranteeData) {
      ptemp=lpRoleGranteeData->pnext;
      ESL_FreeMem(lpRoleGranteeData);  
      lpRoleGranteeData = ptemp;
   }
   return TRUE;
}

BOOL freeRolesData(lpRoleData)
struct RoleData * lpRoleData;
{

   struct RoleData *ptemp;

   while(lpRoleData) {
      ptemp=lpRoleData->pnext;
      freeRoleGranteesData(lpRoleData->lpRoleGranteeData);
      ESL_FreeMem(lpRoleData);  
      lpRoleData = ptemp;
   }
   return TRUE;
}

BOOL freeMonNodeData(lpMonNodeData)
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

BOOL freeNodeServerClassData(lpNodeServerClassData)
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

BOOL freeNodeLoginData(lpNodeLoginData)
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

BOOL freeNodeConnectionData(lpNodeConnectionData)
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

BOOL freeNodeAttributeData(lpNodeAttributeData)
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

BOOL freeWindowData(lpWindowData)
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
