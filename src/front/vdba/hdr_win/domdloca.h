/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : domdloca.h
//    for all domdxxx sources managing cache
//
**   26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**   09-May-2001 (schph01)
**     SIR 102509 Add iRawBlocks in LocationData structure to managing
**     raw data location in cache.
**   06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
**  23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
**  25-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  11-May-2010 (drivi01)
**   Add TableVWType to TableData structure to mark 
**   Ingres VectorWise table structures.
********************************************************************/

#ifndef LIBMON_HEADER
#include "dbatime.h"
#include "monitor.h"
#endif

#define INFOFROMLL        0
#define INFOFROMPARENT    1
#define INFOFROMPARENTIMM 2

typedef struct WindowData  {

   WINDOWDATAMIN WindowDta;

   struct WindowData  * pnext;
} NODEWINDOWDATA, FAR * LPNODEWINDOWDATA;

typedef struct NodeServerClassData  {

   NODESVRCLASSDATAMIN NodeSvrClassDta;

   struct NodeServerClassData  * pnext;
}  NODESERVERCLASSDATA, FAR * LPNODESERVERCLASSDATA;

typedef struct NodeLoginData  {

   NODELOGINDATAMIN NodeLoginDta;

   struct NodeLoginData  * pnext;
} NODELOGINDATA, FAR * LPNODELOGINDATA;

typedef struct NodeConnectionData {

   NODECONNECTDATAMIN NodeConnectionDta;

   struct NodeConnectionData * pnext;
} NODECONNECTDATA, FAR * LPNODECONNECTDATA;

typedef struct NodeAttributeData {

   NODEATTRIBUTEDATAMIN NodeAttributeDta;

   struct NodeAttributeData * pnext;
} NODEATTRIBUTEDATA, FAR * LPNODEATTRIBUTEDATA;

typedef struct MonNodeData {

   NODEDATAMIN MonNodeDta;

   DOMREFRESHPARAMS ServerClassRefreshParms;
   BOOL HasSystemServerClasses;
   int nbserverclasses;
   int serverclasseserrcode;
   struct NodeServerClassData * lpServerClassData;

   DOMREFRESHPARAMS WindowRefreshParms;
   BOOL HasSystemWindows;
   int nbwindows;
   int windowserrcode;
   struct WindowData  * lpWindowData;

   DOMREFRESHPARAMS LoginRefreshParms;
   BOOL HasSystemLogins;
   int nblogins;
   int loginerrcode;
   struct NodeLoginData  * lpNodeLoginData;

   DOMREFRESHPARAMS NodeConnectionRefreshParms;
   BOOL HasSystemNodeConnections;
   int nbNodeConnections;
   int NodeConnectionerrcode;
   struct NodeConnectionData  * lpNodeConnectionData;

   DOMREFRESHPARAMS NodeAttributeRefreshParms;
   BOOL HasSystemNodeAttributes;
   int nbNodeAttributes;
   int NodeAttributeerrcode;
   struct NodeAttributeData  * lpNodeAttributeData;

   struct MonNodeData * ptemp;

   struct MonNodeData * pnext;
} MONNODEDATA, FAR * LPMONNODEDATA;
//  monitoring

typedef struct ServerData {

   SERVERDATAMIN ServerDta;

   DOMREFRESHPARAMS ServerSessionsRefreshParms;
   BOOL   HasSystemServerSessions;
   int    nbServerSessions;
   int    ServerSessionserrcode;
   struct SessionData  * lpServerSessionData;

   DOMREFRESHPARAMS IceConnUsersRefreshParms;
   BOOL   HasSystemIceConnUsers;
   int    nbiceconnusers;
   int    iceConnuserserrcode;
   struct IceConnUserData  * lpIceConnUsersData;

   DOMREFRESHPARAMS IceActiveUsersRefreshParms;
   BOOL   HasSystemIceActiveUsers;
   int    nbiceactiveusers;
   int    iceactiveuserserrcode;
   struct IceActiveUserData  * lpIceActiveUsersData;

   DOMREFRESHPARAMS IceTransactionsRefreshParms;
   BOOL   HasSystemIceTransactions;
   int    nbicetransactions;
   int    icetransactionserrcode;
   struct IceTransactData  * lpIceTransactionsData;

   DOMREFRESHPARAMS IceCursorsRefreshParms;
   BOOL   HasSystemIceCursors;
   int    nbicecursors;
   int    icecursorsserrcode;
   struct IceCursorData  * lpIceCursorsData;

   DOMREFRESHPARAMS IceCacheRefreshParms;
   BOOL   HasSystemIceCacheInfo;
   int    nbicecacheinfo;
   int    icecacheinfoerrcode;
   struct IceCacheData  * lpIceCacheInfoData;

   DOMREFRESHPARAMS IceDbConnectRefreshParms;
   BOOL   HasSystemIceDbConnectInfo;
   int    nbicedbconnectinfo;
   int    icedbconnectinfoerrcode;
   struct IceDbConnectData  * lpIceDbConnectData;

   struct ServerData * ptemp;

   struct ServerData * pnext;
} SERVERDATA, FAR * LPSERVERDATA;

typedef struct SessionData {

   SESSIONDATAMIN SessionDta;

   struct SessionData * ptemp;

   struct SessionData * pnext;

} SESSIONDATA, FAR * LPSESSIONDATA;

typedef struct IceConnUserData {

   ICE_CONNECTED_USERDATAMIN IceConnUserDta;

   struct IceConnUserData * ptemp;

   struct IceConnUserData * pnext;

} ICECONNUSERDATA, FAR * LPICECONNUSERDATA;

typedef struct IceActiveUserData {

   ICE_ACTIVE_USERDATAMIN IceActiveUserDta;

   struct IceActiveUserData * ptemp;

   struct IceActiveUserData * pnext;

} ICEACTIVEUSERDATA, FAR * LPICEACTIVEUSERDATA;

typedef struct IceTransactData {

   ICE_TRANSACTIONDATAMIN IceTransactDta;

   struct IceTransactData * ptemp;

   struct IceTransactData * pnext;

} ICETRANSACTDATA, FAR * LPICETRANSACTDATA;

typedef struct IceCursorData {

   ICE_CURSORDATAMIN IceCursorDta;

   struct IceCursorData * ptemp;

   struct IceCursorData * pnext;

} ICECURSORDATA, FAR * LPICECURSORDATA;

typedef struct IceCacheData {

   ICE_CACHEINFODATAMIN IceCacheDta;

   struct IceCacheData * ptemp;

   struct IceCacheData * pnext;

} ICECACHEDATA, FAR * LPICECACHEDATA;

typedef struct IceDbConnectData {

   ICE_DB_CONNECTIONDATAMIN IceDbConnectDta;

   struct IceDbConnectData * ptemp;

   struct IceDbConnectData * pnext;

} ICEDBCONNECTDATA, FAR * LPICEDBCONNECTDATA;

typedef struct LockListData {

   LOCKLISTDATAMIN LockListDta;

   DOMREFRESHPARAMS LocksRefreshParms;
   BOOL   HasSystemLocks;
   int    nbLocks;
   int    lockserrcode;
   struct LockData  * lpLockData;

   struct LockListData * ptemp;

   struct LockListData * pnext;
} LOCKLISTDATA, FAR * LPLOCKLISTDATA;

typedef struct LockData {

   LOCKDATAMIN LockDta;

   struct LockData * ptemp;

   struct LockData * pnext;
} LOCKDATA, FAR * LPLOCKDATA;

typedef struct ResourceData {

   RESOURCEDATAMIN ResourceDta;

   DOMREFRESHPARAMS ResourceLocksRefreshParms;
   BOOL   HasSystemResourceLocks;
   int    nbResourceLocks;
   int    ResourceLockserrcode;
   struct LockData  * lpResourceLockData;

   struct ResourceData * ptemp;

   struct ResourceData * pnext;
} RESOURCEDATA, FAR * LPRESOURCEDATA;

typedef struct LogProcessData {

   LOGPROCESSDATAMIN LogProcessDta;

   struct LogProcessData * ptemp;

   struct LogProcessData * pnext;
} LOGPROCESSDATA, FAR * LPLOGPROCESSDATA;

typedef struct LogDBData {

   LOGDBDATAMIN LogDBDta;

   struct LogDBData * ptemp;

   struct LogDBData * pnext;
} LOGDBDATA, FAR * LPLOGDBDATA;

typedef struct LogTransactData {

   LOGTRANSACTDATAMIN LogTransactDta;

   struct LogTransactData * ptemp;

   struct LogTransactData * pnext;
} LOGTRANSACTDATA, FAR * LPLOGTRANSACTDATA;

typedef struct MonReplServerData {

   REPLICSERVERDATAMIN MonReplServerDta;

   struct MonReplServerData * ptemp;

   struct MonReplServerData * pnext;
} MONREPLSERVERDATA, FAR * LPMONREPLSERVERDATA;

// end of monitoring


// REPLICATION

struct ReplicConnectionData {
   int ReplicVersion;   // should be REPLIC_V10 or REPLIC_V11

   int DBNumber;
   int ServerNumber; // only if REPLIC_V10
   int TargetType;   // only if REPLIC_V10

   UCHAR NodeName[MAXOBJECTNAME];
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR DBMSType[MAXOBJECTNAME]; // only if REPLIC_V11

   struct ReplicConnectionData * pnext;
};

struct ReplicMailErrData {

   UCHAR UserName[80];

   struct ReplicMailErrData * pnext;
};

struct RawReplicCDDSData {
   int CDDSNo;
   int LocalDB;
   int SourceDB;
   int TargetDB;

   struct RawReplicCDDSData * pnext;
};

struct ReplicRegTableData {
   UCHAR TableName[MAXOBJECTNAME];
   UCHAR TableOwner[MAXOBJECTNAME];
   int CDDSNo;
   BOOL bTablesCreated;
   BOOL bColumnsRegistered;
   BOOL bRulesCreated;

   struct ReplicRegTableData * pnext;
};

struct ReplicServerData {
   int ServerNo;
   UCHAR ServerName[MAXOBJECTNAME];

   struct ReplicServerData * pnext;
};

struct IntegrityData {
   long  IntegrityNumber;
   UCHAR IntegrityName[MAXOBJECTNAME];
   UCHAR IntegrityOwner[MAXOBJECTNAME];

   struct IntegrityData * pnext;
};

struct ColumnData {
   UCHAR ColumnName[MAXOBJECTNAME];
   int   ColumnType;
   int   ColumnLen;
   BOOL  bWithDef;
   BOOL  bWithNull;

   struct ColumnData * pnext;
};

struct RuleData {
   UCHAR RuleName[MAXOBJECTNAME];
   UCHAR RuleOwner[MAXOBJECTNAME];
   UCHAR RuleProcedure[MAXOBJECTNAME];
   UCHAR RuleText[MAXOBJECTNAME];

   struct RuleData * pnext;
};

struct SecurityData {
   UCHAR SecurityUser[MAXOBJECTNAME];
   long  SecurityNumber;
   int   SecurityType;

   struct SecurityData * pnext;
};

struct RawSecurityData {
   UCHAR RSecurityName[MAXOBJECTNAME];
   UCHAR RSecurityTable[MAXOBJECTNAME];
   UCHAR RSecurityTableOwner[MAXOBJECTNAME];
   UCHAR RSecurityUser[MAXOBJECTNAME];
   UCHAR RSecurityDBEvent[MAXOBJECTNAME]; /* includes the owner */
   long  RSecurityNumber;
   int   RSecurityType;

   struct RawSecurityData *pnext;
};

struct IndexData {
   UCHAR IndexName[MAXOBJECTNAME];
   UCHAR IndexOwner[MAXOBJECTNAME];
   UCHAR IndexStorage[MAXOBJECTNAME];
   long  IndexId;

   struct IndexData *pnext;
};

struct GranteeData {
   UCHAR Grantee[MAXOBJECTNAME];
   int   GranteeUGRType;  /* OT_USER, OT_GROUP, OT_ROLE, or OT_ERR */
   int   GranteeType;     /* TBGRANTSEL for example */

   struct GranteeData *pnext;
};

struct RawGranteeData {   /*for all grantees except db grantees*/
   UCHAR GranteeObject[MAXOBJECTNAME];
   UCHAR GranteeObjectOwner[MAXOBJECTNAME];
   int   GranteeObjectType;
   UCHAR Grantee[MAXOBJECTNAME];
   int   GranteeUGRType;  /* OT_USER, OT_GROUP, OT_ROLE, or OT_ERR */
   int   GrantType;     /* TBGRANTSEL for example */

   struct RawGranteeData *pnext;
};

struct RawDBGranteeData {   /*for all grantees except db grantees*/
   UCHAR Database[MAXOBJECTNAME];
   UCHAR Grantee [MAXOBJECTNAME];
   int   GrantType;
   int   GranteeUGRType;  /* OT_USER, OT_GROUP, OT_ROLE, or OT_ERR */
   int   GrantExtraInt;

   struct RawDBGranteeData *pnext;
};

struct DBEventData {
   UCHAR DBEventName[MAXOBJECTNAME];
   UCHAR DBEventOwner[MAXOBJECTNAME];

   struct DBEventData * pnext;
};

struct TableData {
   UCHAR TableName[MAXOBJECTNAME];
   UCHAR TableOwner[MAXOBJECTNAME];
   long  Tableid;
   int   TableStarType;
   int	 TableVWType; //Type of VectorWise structure if VW table

   /* any change not related to sub-branches should be ported into the
      ViewTableData structure */
   DOMREFRESHPARAMS IntegritiesRefreshParms;
   BOOL HasSystemIntegrities;
   BOOL integritiesused;
   int nbintegrities;
   int integritieserrcode;
   int iIntegrityDataQuickInfo;
   struct IntegrityData *lpIntegrityData;

   DOMREFRESHPARAMS ColumnsRefreshParms;
   BOOL HasSystemColumns;
   int nbcolumns;
   int columnserrcode;
   struct ColumnData *lpColumnData;

   DOMREFRESHPARAMS RulesRefreshParms;
   BOOL HasSystemRules;
   int nbRules;
   int ruleserrcode;
   struct RuleData *lpRuleData;

   DOMREFRESHPARAMS SecuritiesRefreshParms;
   BOOL HasSystemSecurities;
   int nbSecurities;
   int securitieserrcode;
   struct SecurityData *lpSecurityData;

   DOMREFRESHPARAMS IndexesRefreshParms;
   BOOL HasSystemIndexes;
   BOOL indexesused;
   int nbIndexes;
   int indexeserrcode;
   int iIndexDataQuickInfo;
   struct IndexData *lpIndexData;

   DOMREFRESHPARAMS TableLocationsRefreshParms;
   BOOL HasSystemTableLocations;
   BOOL tablelocationsused;
   int nbTableLocations;
   int tablelocationserrcode;
   int iTableLocDataQuickInfo;
   struct LocationData *lpTableLocationData;

   DOMREFRESHPARAMS GranteesRefreshParms;
   BOOL HasSystemGrantees;
   int nbGrantees;
   int Granteeserrcode;
   struct GranteeData *lpGranteeData;

   struct TableData * ptemp;

   struct TableData * pnext;
};

struct ViewTableData {
   UCHAR TableName[MAXOBJECTNAME];
   UCHAR TableOwner[MAXOBJECTNAME];

   struct ViewTableData * pnext;
};

struct ViewData {
   UCHAR ViewName[MAXOBJECTNAME];
   UCHAR ViewOwner[MAXOBJECTNAME];
   long  Tableid;  // needed for monitoring
   int   ViewStarType;
   BOOL  bSQL;

   DOMREFRESHPARAMS ViewTablesRefreshParms;
   BOOL HasSystemViewTables;
   int nbViewTables;
   int viewtableserrcode;
   struct ViewTableData *lpViewTableData;

   DOMREFRESHPARAMS ColumnsRefreshParms;
   BOOL HasSystemColumns;
   int nbcolumns;
   int columnserrcode;
   struct ColumnData *lpColumnData;

   DOMREFRESHPARAMS GranteesRefreshParms;
   BOOL HasSystemGrantees;
   int nbGrantees;
   int Granteeserrcode;
   struct GranteeData *lpGranteeData;

   struct ViewData * ptemp;

   struct ViewData * pnext;
};

struct ProcedureData {
   UCHAR ProcedureName[MAXOBJECTNAME];
   UCHAR ProcedureOwner[MAXOBJECTNAME];
   UCHAR ProcedureText[MAXOBJECTNAME];

   struct ProcedureData * pnext;
};

struct SequenceData {
   UCHAR SequenceName[MAXOBJECTNAME];
   UCHAR SequenceOwner[MAXOBJECTNAME];

   struct SequenceData * pnext;
};
struct SynonymData {
   UCHAR SynonymName[MAXOBJECTNAME];
   UCHAR SynonymOwner[MAXOBJECTNAME];
   UCHAR SynonymTable[MAXOBJECTNAME]; /* contains the owner */

   struct SynonymData * pnext;
};

struct DBData {

   UCHAR DBName[MAXDBNAME];
   UCHAR DBOwner[MAXDBNAME];
   long  DBid;
   int   DBType;  /* normal, star or coordinator */
   UCHAR CDBName[MAXDBNAME];  /* coordinator database (for Star databases) */

   DOMREFRESHPARAMS TablesRefreshParms;
   BOOL HasSystemTables;
   int nbtables;
   int tableserrcode;
   struct TableData * lpTableData;

   DOMREFRESHPARAMS ViewsRefreshParms;
   BOOL HasSystemViews;
   int nbviews;
   int viewserrcode;
   struct ViewData  * lpViewData;

   DOMREFRESHPARAMS ProceduresRefreshParms;
   BOOL HasSystemProcedures;
   int nbProcedures;
   int procedureserrcode;
   struct ProcedureData  * lpProcedureData;

   DOMREFRESHPARAMS SequencesRefreshParms;
   BOOL HasSystemSequences;
   int nbSequences;
   int sequenceserrcode;
   struct SequenceData  * lpSequenceData;

   DOMREFRESHPARAMS SchemasRefreshParms;
   BOOL HasSystemSchemas;
   int nbSchemas;
   int schemaserrcode;
   struct SchemaUserData  * lpSchemaUserData;

   DOMREFRESHPARAMS SynonymsRefreshParms;
   BOOL HasSystemSynonyms;
   int nbSynonyms;
   int synonymserrcode;
   struct SynonymData  * lpSynonymData;

   DOMREFRESHPARAMS DBEventsRefreshParms;
   BOOL HasSystemDBEvents;
   int nbDBEvents;
   int dbeventserrcode;
   struct DBEventData  * lpDBEventData;

   DOMREFRESHPARAMS DBGranteesRefreshParms;
   BOOL HasSystemDNGrantees;
   int nbDBGrantees;
   int dbgranteeserrcode;
   struct GranteeData  * lpDBGranteeData;

   DOMREFRESHPARAMS RawGranteesRefreshParms;
   BOOL HasSystemRawGrantees;
   int nbRawGrantees;
   int rawgranteeserrcode;
   struct RawGranteeData  * lpRawGranteeData;

   DOMREFRESHPARAMS RawSecuritiesRefreshParms;
   BOOL HasSystemRawSecurities;
   int nbRawSecurities;
   int rawsecuritieserrcode;
   struct RawSecurityData  * lpRawSecurityData;

   DOMREFRESHPARAMS ReplicConnectionsRefreshParms;
   BOOL HasSystemReplicConnections;
   int nbReplicConnections;
   int replicconnectionserrcode;
   struct ReplicConnectionData  * lpReplicConnectionData;

   DOMREFRESHPARAMS ReplicMailErrsRefreshParms;
   BOOL HasSystemReplicMailErrs;
   int nbReplicMailErrs;
   int replicmailerrserrcode;
   struct ReplicMailErrData  * lpReplicMailErrData;

   DOMREFRESHPARAMS RawReplicCDDSRefreshParms;
   BOOL HasSystemRawReplicCDDS;
   int nbRawReplicCDDS;
   int rawrepliccddserrcode;
   struct RawReplicCDDSData  * lpRawReplicCDDSData;

   DOMREFRESHPARAMS ReplicRegTablesRefreshParms;
   BOOL HasSystemReplicRegTables;
   int nbReplicRegTables;
   int replicregtableserrcode;
   struct ReplicRegTableData  * lpReplicRegTableData;

   DOMREFRESHPARAMS ReplicServersRefreshParms;
   BOOL HasSystemReplicServers;
   int nbReplicServers;
   int replicserverserrcode;
   struct ReplicServerData  * lpReplicServerData;

   DOMREFRESHPARAMS OIDTDBGranteeRefreshParms;
   BOOL HasSystemOIDTDBGrantees;
   int nbOIDTDBGrantees;
   int OIDTDBGranteeerrcode;
   struct RawDBGranteeData  * lpOIDTDBGranteeData;

   DOMREFRESHPARAMS MonReplServerRefreshParms;
   BOOL HasSystemMonReplServers;
   int nbmonreplservers;
   int monreplservererrcode;
   struct MonReplServerData  * lpMonReplServerData;

   struct DBData * ptemp;

   struct DBData * pnext;
};

struct SchemaUserData { 
   UCHAR SchemaUserName [MAXOBJECTNAME];
   UCHAR SchemaUserOwner[MAXOBJECTNAME];

   struct SchemaUserData * pnext;
};

struct ProfileData { 
   UCHAR ProfileName[MAXOBJECTNAME];

   struct ProfileData * pnext;
};

struct UserData { 
   UCHAR UserName[MAXOBJECTNAME];

   BOOL bAuditAll;

   struct UserData * pnext;
};

struct SimpleLeafObjectData {    // generic for objects having only a name / no sub-branches
   UCHAR ObjectName[MAXOBJECTNAME];

   BOOL b1;  // BusunitRole: bExecDoc    BusUnitWebUser:bUnitRead    DocRole/User:bExec
   BOOL b2;  // BusunitRole: bCreateDoc  BusUnitWebUser:bCreateDoc   DocRole/User:bRead
   BOOL b3;  // BusunitRole: bReadDoc    BusUnitWebUser:bReadDoc     DocRole/User:bUpdate
   BOOL b4;  // BusunitRole: <unused>    BusUnitWebUser:<unused>     DocRole/User:bDelete

   struct SimpleLeafObjectData * pnext;
};

struct IceObjectWithRolesAndDBData { 
   UCHAR ObjectName[MAXOBJECTNAME];

   DOMREFRESHPARAMS ObjectRolesRefreshParms;
   BOOL HasSystemObjectRoles;
   int nbobjectroles;
   int objectroleserrcode;
   struct SimpleLeafObjectData  * lpObjectRolesData;

   DOMREFRESHPARAMS ObjectDbRefreshParms;
   BOOL HasSystemObjectDbs;
   int nbobjectdbs;
   int objectdbserrcode;
   struct SimpleLeafObjectData  * lpObjectDbData;

   struct IceObjectWithRolesAndDBData * ptemp;

   struct IceObjectWithRolesAndDBData * pnext;
};

struct IceObjectWithRolesAndUsersData { 
   UCHAR ObjectName[MAXOBJECTNAME];

   DOMREFRESHPARAMS ObjectRolesRefreshParms;
   BOOL HasSystemObjectRoles;
   int nbobjectroles;
   int objectroleserrcode;
   struct SimpleLeafObjectData  * lpObjectRolesData;

   DOMREFRESHPARAMS ObjectUsersRefreshParms;
   BOOL HasSystemObjectUsers;
   int nbobjectusers;
   int objectuserserrcode;
   struct SimpleLeafObjectData  * lpObjectUsersData;

   struct IceObjectWithRolesAndUsersData * ptemp;

   struct IceObjectWithRolesAndUsersData * pnext;
};

struct IceBusinessUnitsData { 
   UCHAR BusinessunitName[MAXOBJECTNAME];

   DOMREFRESHPARAMS ObjectRolesRefreshParms;
   BOOL HasSystemObjectRoles;
   int nbobjectroles;
   int objectroleserrcode;
   struct SimpleLeafObjectData  * lpObjectRolesData;

   DOMREFRESHPARAMS ObjectUsersRefreshParms;
   BOOL HasSystemObjectUsers;
   int nbobjectusers;
   int objectuserserrcode;
   struct SimpleLeafObjectData  * lpObjectUsersData;

   DOMREFRESHPARAMS FacetRefreshParms;
   BOOL HasSystemFacets;
   int nbfacets;
   int facetserrcode;
   struct IceObjectWithRolesAndUsersData  * lpObjectFacetsData;

   DOMREFRESHPARAMS PagesRefreshParms;
   BOOL HasSystemPages;
   int nbpages;
   int pageserrcode;
   struct IceObjectWithRolesAndUsersData  * lpObjectPagesData;

   DOMREFRESHPARAMS ObjectLocationsRefreshParms;
   BOOL HasSystemObjectLocations;
   int nbobjectlocations;
   int objectlocationserrcode;
   struct SimpleLeafObjectData  * lpObjectLocationsData;

   struct IceBusinessUnitsData * ptemp;

   struct IceBusinessUnitsData * pnext;
};


struct GroupUserData { 
   UCHAR GroupUserName[MAXOBJECTNAME];
   struct GroupUserData * pnext;
};

struct GroupData { 
   UCHAR GroupName[MAXOBJECTNAME];

   DOMREFRESHPARAMS GroupUsersRefreshParms;
   BOOL HasSystemGroupUsers;
   int nbgroupusers;
   int groupuserserrcode;
   struct GroupUserData  * lpGroupUserData;

   struct GroupData * ptemp;

   struct GroupData * pnext;
};

struct RoleGranteeData { 
   UCHAR RoleGranteeName[MAXOBJECTNAME];
   struct RoleGranteeData * pnext;
};

struct RoleData { 
   UCHAR RoleName[MAXOBJECTNAME];

   DOMREFRESHPARAMS RoleGranteesRefreshParms;
   BOOL HasSystemRoleGrantees;
   int nbrolegrantees;
   int rolegranteeserrcode;
   struct RoleGranteeData  * lpRoleGranteeData;

   struct RoleData * ptemp;

   struct RoleData * pnext;
};

struct LocationData {
  UCHAR LocationName[MAXOBJECTNAME];
  UCHAR LocationAreaStart[MAXOBJECTNAME];

  UCHAR LocationUsages[5];
  int   iRawPct;
  struct LocationData * pnext;
};

struct ServConnectData {

   UCHAR VirtNodeName[MAXOBJECTNAME];

   UCHAR  ICEUserName     [MAXOBJECTNAME];
   UCHAR  ICEUserPassword [MAXOBJECTNAME];

   DOMREFRESHPARAMS DBRefreshParms;
   BOOL HasSystemDB;
   int nbdb;
   int DBerrcode;
   struct DBData    *lpDBData;

   DOMREFRESHPARAMS ProfilesRefreshParms;
   BOOL HasSystemProfiles;
   int nbprofiles;
   int profileserrcode;
   struct ProfileData  *lpProfilesData;

   DOMREFRESHPARAMS UsersRefreshParms;
   BOOL HasSystemUsers;
   int nbusers;
   int userserrcode;
   struct UserData  *lpUsersData;

   DOMREFRESHPARAMS GroupsRefreshParms;
   BOOL HasSystemGroups;
   int nbgroups;
   int groupserrcode;
   struct GroupData *lpGroupsData;

   DOMREFRESHPARAMS RolesRefreshParms;
   BOOL HasSystemRoles;
   int nbroles;
   int roleserrcode;
   struct RoleData  *lpRolesData;

   DOMREFRESHPARAMS LocationsRefreshParms;
   BOOL HasSystemLocations;
   int nblocations;
   int locationserrcode;
   struct LocationData *lpLocationsData;

   DOMREFRESHPARAMS RawDBSecuritiesRefreshParms;
   BOOL HasSystemRawDBSecurities;
   int nbRawDBSecurities;
   int rawdbsecuritieserrcode;
   struct RawSecurityData  * lpRawDBSecurityData;

   DOMREFRESHPARAMS RawDBGranteesRefreshParms;
   BOOL HasSystemRawDBGrantees;
   int nbRawDBGrantees;
   int Rawdbgranteeserrcode;
   struct RawDBGranteeData  * lpRawDBGranteeData;

   DOMREFRESHPARAMS ServerRefreshParms;
   BOOL HasSystemServers;
   int nbservers;
   int serverserrcode;
   struct ServerData    *lpServerData;

   DOMREFRESHPARAMS LockListRefreshParms;
   BOOL HasSystemLockLists;
   int nblocklists;
   int LockListserrcode;
   struct LockListData    *lpLockListData;

   DOMREFRESHPARAMS ResourceRefreshParms;
   BOOL HasSystemResources;
   int nbresources;
   int resourceserrcode;
   struct ResourceData    *lpResourceData;

   DOMREFRESHPARAMS LogProcessesRefreshParms;
   BOOL   HasSystemLogProcesses;
   int    nbLogProcesses;
   int    logprocesseserrcode;
   struct LogProcessData  * lpLogProcessData;

   DOMREFRESHPARAMS LogDBRefreshParms;
   BOOL   HasSystemLogDB;
   int    nbLogDB;
   int    logDBerrcode;
   struct LogDBData  * lpLogDBData;

   DOMREFRESHPARAMS LogTransactRefreshParms;
   BOOL   HasSystemLogTransact;
   int    nbLogTransact;
   int    LogTransacterrcode;
   struct LogTransactData  * lpLogTransactData;

   DOMREFRESHPARAMS IceRolesRefreshParms;
   BOOL HasSystemIceRoles;
   int nbiceroles;
   int iceroleserrcode;
   struct SimpleLeafObjectData  * lpIceRolesData;

   DOMREFRESHPARAMS IceDbUsersRefreshParms;
   BOOL HasSystemIceDbUsers;
   int nbicedbusers;
   int icedbuserserrcode;
   struct SimpleLeafObjectData  * lpIceDbUsersData;

   DOMREFRESHPARAMS IceDbConnectRefreshParms;
   BOOL HasSystemIceDbConnections;
   int nbicedbconnections;
   int icedbconnectionserrcode;
   struct SimpleLeafObjectData  * lpIceDbConnectionsData;

   DOMREFRESHPARAMS IceWebUsersRefreshParms;
   BOOL HasSystemIceWebusers;
   int nbicewebusers;
   int icewebusererrcode;
   struct IceObjectWithRolesAndDBData  * lpIceWebUsersData;

   DOMREFRESHPARAMS IceProfilesRefreshParms;
   BOOL HasSystemIceProfiles;
   int nbiceprofiles;
   int iceprofileserrcode;
   struct IceObjectWithRolesAndDBData  * lpIceProfilesData;

   DOMREFRESHPARAMS IceBusinessUnitsRefreshParms;
   BOOL HasSystemIceBusinessUnits;
   int nbicebusinessunits;
   int icebusinessunitserrcode;
   struct IceBusinessUnitsData  * lpIceBusinessUnitsData;

   DOMREFRESHPARAMS IceApplicationRefreshParms;
   BOOL HasSystemIceApplications;
   int nbiceapplications;
   int iceapplicationserrcode;
   struct SimpleLeafObjectData  * lpIceApplicationsData;

   DOMREFRESHPARAMS IceLocationRefreshParms;
   BOOL HasSystemIceLocations;
   int nbicelocations;
   int icelocationserrcode;
   struct SimpleLeafObjectData  * lpIceLocationsData;

   DOMREFRESHPARAMS IceVarRefreshParms;
   BOOL HasSystemIceVars;
   int nbicevars;
   int icevarserrcode;
   struct SimpleLeafObjectData  * lpIceVarsData;
};

#define MAXNODES 200

struct def_virtnode  {
                       int      nUsers;
                       BOOL     bIsSpecialMonState;
                       struct ServConnectData *pdata;           
                     };

extern struct def_virtnode  virtnode[MAXNODES];

extern DOMREFRESHPARAMS MonNodesRefreshParms;
extern BOOL HasSystemMonNodes;
extern int nbMonNodes;
extern int monnodeserrcode;
extern struct MonNodeData   * lpMonNodeData;

extern int imaxnodes;

struct IntegrityData   * GetNextIntegrity(struct IntegrityData * pintegritydata);
struct ColumnData      * GetNextColumn(struct ColumnData * pcolumndata);
struct RuleData        * GetNextRule(struct RuleData * pruledata);
struct SecurityData    * GetNextSecurity(struct SecurityData * psecuritydata);
struct RawSecurityData * GetNextRawSecurity(struct RawSecurityData * prawsecuritydata);
struct IndexData       * GetNextIndex(struct IndexData * pindexdata);
struct GranteeData     * GetNextGrantee(struct GranteeData * pgranteedata);
struct RawGranteeData  * GetNextRawGrantee(struct RawGranteeData * prawGranteedata);
struct RawDBGranteeData * GetNextRawDBGrantee(struct RawDBGranteeData * prawDBGranteedata);
struct DBEventData     * GetNextDBEvent(struct DBEventData * pdbeventdata);
struct ProcedureData   * GetNextProcedure(struct ProcedureData * pproceduredata);
struct SequenceData    * GetNextSequence(struct SequenceData * psequencedata);
struct SynonymData     * GetNextSynonym(struct SynonymData * psynonymdata);
struct SchemaUserData  * GetNextSchemaUser(struct SchemaUserData * pschemauserdata);
struct TableData       * GetNextTable(struct TableData * pTabledata);
struct ViewData        * GetNextView(struct ViewData * pViewdata);
struct ViewTableData   * GetNextViewTable(struct ViewTableData *pViewTabledata);
struct ProfileData     * GetNextProfile(struct ProfileData * pProfiledata);
struct UserData        * GetNextUser(struct UserData * pUserdata);
struct GroupData       * GetNextGroup(struct GroupData * pGroupdata);
struct GroupUserData   * GetNextGroupUser(struct GroupUserData *pGroupUserdata);
struct RoleGranteeData * GetNextRoleGrantee(struct RoleGranteeData *pRoleGranteedata);
struct RoleData        * GetNextRole(struct RoleData * pRoledata);
struct LocationData    * GetNextLocation(struct LocationData * pLocationdata);
struct DBData          * GetNextDB(struct DBData * pDBdata);
struct NodeServerClassData * GetNextServerClass(struct NodeServerClassData * pServerClassdata);
struct WindowData          * GetNextNodeWindow(struct WindowData   * pWindowData);
struct NodeLoginData       * GetNextNodeLogin(struct NodeLoginData   * pNodeLoginData);
struct NodeConnectionData  * GetNextNodeConnection(struct NodeConnectionData * pNodeConnectionData);
struct NodeAttributeData   * GetNextNodeAttribute(struct NodeAttributeData  * pNodeAttributeData);
struct MonNodeData         * GetNextMonNode(struct MonNodeData * pMonNodeData);
struct MonReplServerData   * GetNextMonReplServer(struct MonReplServerData  * pMonReplServerData);

struct ReplicConnectionData * GetNextReplicConnection(struct ReplicConnectionData * pReplicConnectiondata);
struct ReplicMailErrData    * GetNextReplicMailErr(struct ReplicMailErrData * pReplicMailErrdata);
struct RawReplicCDDSData    * GetNextRawReplicCDDS(struct RawReplicCDDSData * pRawReplicCDDSdata);
struct ReplicRegTableData   * GetNextReplicRegTable(struct ReplicRegTableData * pReplicRegTabledata);
struct ReplicServerData     * GetNextReplicServer(struct ReplicServerData * pReplicServerdata);

struct ServerData * GetNextServer(struct ServerData * pServerdata);
struct SessionData * GetNextSession(struct SessionData * pSessiondata);
struct LockListData * GetNextLockList(struct LockListData * pLockListdata);
struct LockData * GetNextLock(struct LockData * pLockdata);
struct ResourceData * GetNextResource(struct ResourceData * pResourcedata);
struct LogProcessData * GetNextLogProcess(struct LogProcessData * pLogProcessdata);
struct LogDBData * GetNextLogDB(struct LogDBData * pLogDBdata);
struct LogTransactData * GetNextLogTransact(struct LogTransactData * pLogTransactdata);
struct IceConnUserData   * GetNextIceConnUser(struct IceConnUserData * pIceConnectUserdata);
struct IceActiveUserData * GetNextIceActiveUser(struct IceActiveUserData *pIceActiveUserdata);
struct IceTransactData   * GetNextIceTransaction(struct IceTransactData * pIceTransactiondata);
struct IceCursorData     * GetNextIceCursor(struct IceCursorData * pIceCursordata);
struct IceCacheData      * GetNextIceCacheInfo(struct IceCacheData * pIceCachedata);
struct IceDbConnectData  * GetNextIceDbConnectdata(struct IceDbConnectData * pIceDbConnectdata);

// ICE DOM branches
struct SimpleLeafObjectData        * GetNextSimpleLeafObject (struct SimpleLeafObjectData * pSimpleLeafObjectdata);
struct IceObjectWithRolesAndDBData * GetNextObjWithRoleAndDb (struct IceObjectWithRolesAndDBData * pIceObjdata);
struct IceBusinessUnitsData        * GetNextIceBusinessUnit  (struct IceBusinessUnitsData *pIceBusinessUnitsdata);
struct IceObjectWithRolesAndUsersData * GetNextObjWithRoleAndUsr (struct IceObjectWithRolesAndUsersData * pIceObjdata);
                       
BOOL freeIntegritiesData(struct IntegrityData *lpIntegritiesdata);
BOOL freeColumnsData(struct ColumnData *lpColumnsdata);
BOOL freeRulesData(struct RuleData *lpRulesdata);
BOOL freeSecuritiesData(struct SecurityData *lpSecuritiesdata);
BOOL freeRawSecuritiesData(struct RawSecurityData *lpRawSecuritiesdata);
BOOL freeIndexData(struct IndexData *lpIndexdata);
BOOL freeGranteesData(struct GranteeData *lpGranteedata);
BOOL freeRawGranteesData(struct RawGranteeData *lpRawGranteedata);
BOOL freeRawDBGranteesData(struct RawDBGranteeData *lpRawDBGranteedata);
BOOL freeDBEventsData(struct DBEventData *lpDBEventdata);
BOOL freeLocationsData(struct LocationData *lpLocationData);
BOOL freeTablesData(struct TableData *lpTabledata);
BOOL freeViewTablesData(struct ViewTableData *lpViewTabledata);
BOOL freeViewsData(struct ViewData *lpViewdata);
BOOL freeProceduresData(struct ProcedureData  * lpProcedureData);
BOOL freeSequencesData(struct SequenceData  * lpSequenceData);
BOOL freeSynonymsData(struct SynonymData  * lpSynonymData);
BOOL freeSchemaUsersData(struct SchemaUserData  * lpSchemaUserData);
BOOL freeDBData(struct DBData * lpDBdata);
BOOL freeProfilesData(struct ProfileData  * lpProfileData);
BOOL freeUsersData(struct UserData  * lpUserData);
BOOL freeGroupUsersData(struct GroupUserData  * lpGroupUserData);
BOOL freeGroupsData(struct GroupData * lpGroupData);
BOOL freeRoleGranteesData(struct RoleGranteeData  * lpRoleGranteeData);
BOOL freeRolesData(struct RoleData * lpRoleData);

BOOL freeReplicConnectionsData(struct ReplicConnectionData *lpReplicConnectionsdata);
BOOL freeReplicMailErrsData(struct ReplicMailErrData *lpReplicMailErrsdata);
BOOL freeRawReplicCDDSData(struct RawReplicCDDSData  *lpRawReplicCDDSdata);
BOOL freeReplicRegTablesData(struct ReplicRegTableData *lpReplicRegTablesdata);
BOOL freeReplicServersData(struct ReplicServerData *lpReplicServersdata);
BOOL freeMonReplServerData(struct MonReplServerData * lpMonReplServerData);

BOOL freeServerData(struct ServerData * lpServerdata);
BOOL freeSessionData(struct SessionData * lpSessiondata);
BOOL freeLockListData(struct LockListData * lpLockListdata);
BOOL freeLockData(struct LockData * lpLockdata);
BOOL freeResourceData(struct ResourceData * lpResourcedata);
BOOL freeLogProcessData(struct LogProcessData * lpLogProcessdata);
BOOL freeLogDBData(struct LogDBData * lpLogDBdata);
BOOL freeLogTransactData(struct LogTransactData * lpLogTransactdata);
BOOL freeIceConnectUsersData(struct IceConnUserData   *  pIceConnectUsersdata);
BOOL freeIceActiveUsersData (struct IceActiveUserData *  pIceActiveUsersdata);
BOOL freeIcetransactionsData(struct IceTransactData * pIceTransactionsdata);
BOOL freeIceCursorsData     (struct IceCursorData * pIceCursorsdata);
BOOL freeIceCacheData       (struct IceCacheData * pIceCachedata);
BOOL freeIceDbConnectData   (struct IceDbConnectData * pIceDbConnectdata);
// ICE DOM branches
BOOL freeSimpleLeafObjectData       (struct SimpleLeafObjectData * pSimpleLeafObjectdata);
BOOL freeIceObjectWithRolesAndDBData(struct IceObjectWithRolesAndDBData * pIceObjdata);
BOOL freeIceBusinessUnitsData       (struct IceBusinessUnitsData *pIceBusinessUnitsdata);
BOOL freeIceObjectWithRolesAndUsrData(struct IceObjectWithRolesAndUsersData *pIceObjData);


BOOL freeRevInfoData(void);

BOOL freeWindowData         (struct WindowData         * );
BOOL freeNodeServerClassData(struct NodeServerClassData *);
BOOL freeNodeLoginData      (struct NodeLoginData      * );
BOOL freeNodeConnectionData (struct NodeConnectionData * );
BOOL freeNodeAttributeData  (struct NodeAttributeData * );
BOOL freeMonNodeData        (struct MonNodeData        * );


LPMONNODEDATA       GetFirstLLNode         (void);
LPNODECONNECTDATA   GetFirstLLConnectData  (void);
LPNODELOGINDATA     GetFirstLLLoginData    (void);
LPNODEATTRIBUTEDATA GetFirstLLAttributeData(void);


int LLAddFullNode           (LPNODEDATAMIN lpNData);
int LLAddNodeConnectData    (LPNODECONNECTDATAMIN lpNCData);
int LLAddNodeLoginData      (LPNODELOGINDATAMIN lpNLData);
int LLAddNodeAttributeData  (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LLDropFullNode          (LPNODEDATAMIN lpNData);
int LLDropNodeConnectData   (LPNODECONNECTDATAMIN lpNCData);
int LLDropNodeLoginData     (LPNODELOGINDATAMIN lpNLData);
int LLDropNodeAttributeData (LPNODEATTRIBUTEDATAMIN lpAttributeData);
int LLAlterFullNode         (LPNODEDATAMIN lpNDataold,LPNODEDATAMIN lpNDatanew);
int LLAlterNodeConnectData  (LPNODECONNECTDATAMIN lpNCDataold,LPNODECONNECTDATAMIN lpNCDatanew);
int LLAlterNodeLoginData    (LPNODELOGINDATAMIN lpNLDataold,LPNODELOGINDATAMIN lpNLDatanew);
int LLAlterNodeAttributeData(LPNODEATTRIBUTEDATAMIN lpAttributeDataold,LPNODEATTRIBUTEDATAMIN lpAttributeDatanew);
