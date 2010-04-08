/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : CA/OpenIngres Visual DBA
**    Source : dbaset.h
**    all object type structures (for add/alter/drop, drag/drop, ...)
**
**  History:
**  14-Feb-2000 (somas01)
**     Added typedef to DBAAlterObject() prototype. Also, removed
**     extra comma from TableDetailSubset declaration.
**  24-Feb-2000 (uk$so01)
**     Bug #100562, Add "nKeySequence" to the COLUMNPARAMS to indicate if the columns
**     is part of primary key or unique key.
**  22-05-2000 (schph01)
**     bug 99242 (DBCS) remove function GetSQLstmType() never used in VDBA.
**  30-May-2000 (uk$so01)
**     bug 99242 (DBCS)  Change function prototype for DBCS compliant
**  16-Nov-2000 (schph01)
**     sir 102821 Comment on table and columns.
**  08-Dec-2000 (schph01)
**     sir 102823 Add bSystemMaintained in COLUMNPARAMS structure, used with
**               Object_Key or Table_Key Data type.
**  15-Dec-2000 (schph01)
**     sir 102819 Add function prototype SQLGetColumnList() 
**  21-Mar-2001 (noifr01)
**    (sir 104270) removal of definitions for the code managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6
**  12-Apr-2001 (noifr01)
**    (sir 102955) added prototype of new SQLGetPageTypesPerTableTypes()
**    function for getting the info on the page types that will be used vs.
**    the page size and the "table or index" information
**  09-May-2001(schph01)
**     (SIR 102509) to manage the raw data location, add in LocationParams
**     structure the new field iRawBlocks.
**  30-May-2001 (uk$so01)
**     SIR #104736, Integrate IIA and Cleanup old code of populate.
**  06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
**  15-Jun-2001(schph01)
**     SIR 104991 add in USERPARAMS structure two variables to manage the 
**     grants needed for rmcmd.
**  20-Jun-2001(schph01)
**     SIR 103694 to manage UNICODE database type, add in CREATEDB structure
**     a boolean bUnicodeDatabase.
**  27-Jun-2001 (noifr01)
**     (sir 103694) added additional prototypes for unicode management from
**     SQL/Test
**  28-Jun-2001 (schph01)
**     (SIR 105158) Add prototype for SQLGetTableStorageStructure() function, used
**     to retrieve the actuel storage structure of the current table.
**  25-Oct-2001 (noifr01)
**    (sir 106156) added ExecSQLImm1 and ExecSQLImmWithUni1 functions for
**    getting back the number of affected rows
**  23-Nov-2001 (noifr01)
**   added #if[n]def's for the libmon project
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  07-Oct-2002 (schph01)
**     bug 108883 Add iOwnerRmcmdObjects in USERPARAMS structure.
**     this variable can have three values : RMCMDOBJETS_NO_OWNER,
**                                           RMCMDOBJETS_OWNER,
**                                           RMCMDOBJETS_PARTIAL_OWNER
**  25-Mar-2003 (schph01 and noifr01)
**   (sir 107523) management of sequences
**  15-May-2003 (schph01)
**     bug 110247 Add bUnicodeDB in DATABASEPARAMS structure.
**     filled by GetDetailInfo function.
**  03-Nov-2003 (noifr01)
**     fixed errors from massive ingres30->main gui tools source propagation
**  23-Jan-2004 (schph01)
**   (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
**  06-Feb-2004 (schph01)
**    SIR 111752 Add new variable member bGWOracleWithDBevents in GATEWAYCAPABILITIESINFO
**    structure, for managing DBEvent on Gateway.
**  26-May-2004 (schph01)
**    SIR #112460 Add variable member szCatalogsPageSize in CREATEDB and
**    DATABASEPARAMS structures, to manage -page_size option for createdb
**    command.
**  01-Jul-2004 (schph01)
**    SIR #112590 Add variable member bConcurrentUpdates in STORAGEPARAMS
**    strutures, to manage -concurrent_updates option for modify command.
**  07-Apr-2005 (srisu02)
**    Add variable member bRemovePwd in USERPARAMS struture, 
**    for 'Delete password' functionality in 'Alter User' dialog box
**  06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
**  02-Feb-2006 (drivi01)
**    BUG 115691
**    Keep VDBA tools at the same version as DBMS server.  Update VDBA
**    to 9.0 level versioning. Make sure that 9.0 version string is
**    correctly interpreted by visual tools and VDBA internal version
**    is set properly upon that interpretation.
**  12-Oct-2006 (wridu01)
**    (Sir 116835) Add support for Ansi Date/Time data types
**  11-Jan-2010 (drivi01)
**    Add version constant for 10.0 release of Ingres.
**  22-Mar-2010 (drivi01)
**    Add function prototypes for detecting VectorWise installation.
**  23-Mar-2010 (drivi01)
**    Add structure constant for VectorWise table structure.
*/

#ifndef _DBASET_H
#define _DBASET_H

#ifndef LIBMON_HEADER
#include <stdio.h>
#include <stdlib.h>
#include "dba.h"
#endif

// the OIVERS_ definitions must be in ascending order for simplifying criteria against versions
// of Ingres.. This assertion is checked in dbaginfo.sc
#define OIVERS_NOTOI  0
#define OIVERS_12     (OIVERS_NOTOI + 1)
#define OIVERS_20     (OIVERS_NOTOI + 2)
#define OIVERS_25     (OIVERS_NOTOI + 3)
#define OIVERS_26     (OIVERS_NOTOI + 4)
#define OIVERS_30     (OIVERS_NOTOI + 5)
#define OIVERS_90     (OIVERS_NOTOI + 6)
#define OIVERS_91     (OIVERS_NOTOI + 7)
#define OIVERS_100    (OIVERS_NOTOI + 10)

int GetOIVers ();   // Return the OpenIngres version.
int SetOIVers (int oivers);
int IsVW(); //return true if this is VectorWise
void SetVW();

typedef struct tagOBJECTLIST
{
   LPVOID lpNext;             /* Pointer to the next object in the list */
   LPVOID lpObject;           /* Pointer to the object                  */
} OBJECTLIST, FAR * LPOBJECTLIST;

typedef struct tagDD_REGISTERED_TABLES
{
  UCHAR         tablename[MAXOBJECTNAME];  // speak for itself
  UCHAR         tableowner[MAXOBJECTNAME]; // speak for itself
  UCHAR         IndexName[MAXOBJECTNAME];  // Indexe Name Replicator 1.1
  UINT          table_no;                  // Numeric identifier replicator 1.1 only
  UCHAR         table_created[2];          // flag 'T' if tables are created
  UCHAR         table_created_ini[2];      // value before dialog
  UCHAR         colums_registered[2];      // flag 'C' if columns are created
  UCHAR         rules_created[2];          // flag 'R' if rules are created
  UCHAR         rules_created_ini[2];      // values before dialog
  UCHAR         rule_lookup_table[25];     // name of the lookup table
  UCHAR         cdds_lookup_table_v11[33]; // name of the lookup table replicator 1.1
  UCHAR         priority_lookup_table[25]; // name of the priority lookup table
  UCHAR         priority_lookup_table_v11[33]; // name of the priority lookup table replicator 1.1
  UINT          cdds;                      // CDDS for this table
  LPOBJECTLIST  lpCols;                    // Columns registered for this table
  BOOL          bStillInCDDS;              // For Alter CDDS dialog box
  UCHAR         szDate_rules_created [MAXOBJECTNAME]; 
  UCHAR         szDate_table_created [MAXOBJECTNAME]; 
} DD_REGISTERED_TABLES, FAR *LPDD_REGISTERED_TABLES;

typedef struct StarDBComponentParams
{
  UCHAR NodeName    [MAXOBJECTNAME];
  UCHAR DBName      [MAXOBJECTNAME];
  UCHAR ServerClass [MAXOBJECTNAME];
  int   itype;
  UCHAR ObjectName  [MAXOBJECTNAME];
  UCHAR ObjectOwner [MAXOBJECTNAME];
} STARDBCOMPONENTPARAMS, FAR * LPSTARDBCOMPONENTPARAMS;

BOOL ManageSqlError(void *Sqlca);
BOOL AddSqlError(char * statement, char * retmessage, int icode);
void EnableSqlSaveText();
void DisableSqlSaveText();

#ifndef LIBMON_HEADER
typedef struct tagSQLERRORINFO {
   struct tagSQLERRORINFO  *lpNext;
   struct tagSQLERRORINFO  *lpPrev;
   BOOL                    bActiveEntry;
   long                    sqlcode;
   LPUCHAR                 lpSqlStm;
   LPUCHAR                 lpSqlErrTxt;
} SQLERRORINFO, FAR *LPSQLERRORINFO;   
LPSQLERRORINFO GetNewestSqlError();
VOID FreeSqlErrorPtr();
BOOL FillErrorStructure();
LPTSTR VdbaGetTemporaryFileName();
VOID   VdbaSetTemporaryFileName(LPTSTR pFileName);
BOOL   VdbaInitializeTemporaryFileName();
#endif
#define SQLERRORTXTLEN    (1024+1)

typedef struct tagCURRENTSQLERRINFO {
   long                    sqlcode;
   TCHAR                  tcSqlStm[SQLERRORTXTLEN];
   TCHAR                  tcSqlErrTxt[SQLERRORTXTLEN];
} CURRENTSQLERRINFO, FAR *LPCURRENTSQLERRINFO;

int ExecSQLImm(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm);

struct int2string
{
  int item;
  char * pchar;
};

#ifndef LIBMON_HEADER

// Maximum number of columns in a table
#define MAXIMUM_COLUMNS    300


#define SQL_UNKOWN_TYPE  0     
#define SQL_CHAR_TYPE    1  
#define SQL_FLT_TYPE     2         
#define SQL_INT_TYPE     3
#define SQL_VCHAR_TYPE   4
#define SQL_NULL_TYPE    5
#define SQL_DATE_TYPE    6  
#define SQL_BYTE_TYPE    7    // Added Emb Jan 06, 97
#define USE_DEFAULT     -1
#define SQL_SELECT       1
#define SQL_CONNECT      2 
#define SQL_OTHER        3

#define SQL_TYPE_LPSQLROWSET  1
#define SQL_TYPE_SQLROWSET    2
#define SQL_TYPE_SQLchar      3  
#define SQL_TYPE_SQLint       4  

typedef struct tagSTRINGLIST
{
    LPTSTR lpszString;
    short  extraInfo1;
    LONG   extraInfo2;
    LONG   extraInfo3;
    struct tagSTRINGLIST* next;
} STRINGLIST, FAR* LPSTRINGLIST;

/*
** Structure For system informations on a Gateway
**
**
*/
typedef struct GatewayCapabilitiesInfo
{
	BOOL bCreateTableSyntaxeExtend; // TRUE when open/sql syntaxe extended is available
	BOOL bDateTypeAvailable;		// TRUE when data type "date" is available
	BOOL bOwnerTableSupported;		// TRUE when owner.table format is supported
	BOOL bGWOracleWithDBevents;		// TRUE when DBEvents are supported for an Oracle Gateway
}GATEWAYCAPABILITIESINFO , FAR * LPGATEWAYCAPABILITIESINFO;

//typedef unsigned char * LPBYTE;    // to get a byte pointer, care with DBCS

typedef struct IngToDbaType {
   int iIngType;
   int iDbaType;
   int iLen;
} INGDBATYPES, FAR * LPINGDBATYPES;

// convention for error occuring while background task is in progress,
// value used in hwndMdiActive field
//#define BKTASK_FAKE_HANDLE 0


typedef struct __HOSTVAR {
   int  itype;
   int  isize;
   void *lpHost;
} SQL_HOSTVAR, FAR * LPSQL_HOSTVAR;

#define DEFHOST(type,name) \
type name; \
SQL_HOSTVAR name##_h = {SQL_TYPE_##type, sizeof(name), (void *) &(name)}

#define DEFSTRINGHOST(name,len) \
char name[len]; \
SQL_HOSTVAR name##_h = {SQL_TYPE_char, sizeof(name), (void *) &(name)}

#define HOSTVAR(name) &(name##_h)
 
typedef LONG HSQL;

typedef struct checkedobjects {
   UCHAR dbname[MAXOBJECTNAME*2]; // Double this buffer for double quoted management. PS 15/02/1999
   BOOL  bchecked;
   struct checkedobjects *pnext;
} CHECKEDOBJECTS, FAR * LPCHECKEDOBJECTS;

typedef struct tagCREATEDB
{
    UCHAR szDBName[MAXDBNAME];          // Database name
    UCHAR szUserName[MAXOBJECTNAME];    // User name
    UCHAR Language[EXTRADATASIZE];      // Collating sequence for database
    UCHAR Products[EXTRADATASIZE];      // Specific products to create catalogs for remote command

    UCHAR LocDatabase[MAXOBJECTNAME];   // Location for database
    UCHAR LocCheckpoint[MAXOBJECTNAME]; // Location for checkpoint
    UCHAR LocJournal[MAXOBJECTNAME];    // Location for journal
    UCHAR LocDump[MAXOBJECTNAME];       // Location for dump
    UCHAR LocWork[MAXOBJECTNAME];       // Location for work
    BOOL bPrivate;                      // TRUE if private database
    BOOL bAlter;                        // TRUE for Alter Database dialog

  // Star management
    BOOL  bDistributed;                 // TRUE if star database
    UCHAR szCoordName[MAXDBNAME];       // Coordinator db name

    BOOL  bGenerateFrontEnd;            // TRUE if at least one fe product, FALSE for nofe catalogs
    BOOL  bAllFrontEnd;                 // TRUE if all fe products selected
    UCHAR szCatOIToolsAvailables[MAX_TOOL_PRODUCTS+1]; // list of OI tools catalogs installed
  // Location Extension management
    LPOBJECTLIST lpLocExt;

    BOOL bReadOnly;
  // Unicode database type Enable
    BOOL bUnicodeDatabaseNFD;
    BOOL bUnicodeDatabaseNFC;
    UCHAR szCatalogsPageSize[MAXDBNAME];       // Catalogs Page Size

} CREATEDB, FAR * LPCREATEDB;

#define LOCATIONUSAGES     7

#define LOCATIONDATABASE   0 /* should be < LOCATIONUSAGES          */
#define LOCATIONWORK       1 /* used as position in bLocationUsage[]*/
#define LOCATIONJOURNAL    2 /* in LocationParams structure         */
#define LOCATIONCHECKPOINT 3
#define LOCATIONDUMP       4
#define LOCATIONALL        5
#define LOCATIONNOUSAGE    6
                      
typedef struct LocationParams {
   UCHAR objectname[MAXOBJECTNAME];
   UCHAR AreaName[MAXAREANAME];
   BOOL  bLocationUsage[LOCATIONUSAGES];
   int   iRawPct;

   BOOL bCreate;
} LOCATIONPARAMS, FAR * LPLOCATIONPARAMS;

#define USERSECAUDITS       3
                                                                     
#define USERALLEVENT        0 /* should be < USERSECAUDITS since they are  */
#define USERDEFAULTEVENT    1 /* used as position in bSecAusit             */
#define USERQUERYTEXTEVENT  2 /* in UserParams and ProfileParams strutures */
                              

#define USERPRIVILEGES    14

#define USERCREATEDBPRIV   0 /* should be < USERPRIVILEGES          */
#define USERTRACEPRIV      1 /* used as position in Privilege[]     */
#define USERSECURITYPRIV   2 /* in UserParams and ProfileParams     */
#define USEROPERATORPRIV   3 /* structures                          */ 
#define USERMAINTLOCPRIV   4
#define USERAUDITALLPRIV   5
#define USERMAINTAUDITPRIV 6
#define USERMAINTUSERPRIV  7
// ------- new 28-10-1999: MAC Priv
#define USERWRITEDOWN             8
#define USERWRITEFIXED            9
#define USERWRITEUP              10
#define USERINSERTDOWN           11
#define USERINSERTUP             12
#define USERSESSIONSECURITYLABEL 13

enum
{
   RMCMDOBJETS_NO_OWNER,
   RMCMDOBJETS_OWNER,
   RMCMDOBJETS_PARTIAL_OWNER
};

typedef struct UserParams {
   UCHAR ObjectName[MAXOBJECTNAME];
   UCHAR DefaultGroup[MAXOBJECTNAME];
   BOOL  Privilege[USERPRIVILEGES];
   BOOL  bDefaultPrivilege[USERPRIVILEGES]; // True if Default privilege[USERxxxPRIV]..
                                            // ..is checked. All FALSE if NODEFAULT_PRIVILEGES
   LPCHECKEDOBJECTS lpfirstdbIn;
   LPCHECKEDOBJECTS lpfirstdb;
   BOOL  bSecAudit[USERSECAUDITS];        // TRUE if bSecAudit[USERxxxSECAUD] is checked. 
   UCHAR ExpireDate[MAXDATELEN];          // expire date or "\0" if none
   LPUCHAR lpLimitSecLabels;              // Limiting sec labels NULL if ...
                                          // .. nolimiting_security_label)..
                                          // .. MAX length = MAXLIMITSECLBL
   UCHAR szProfileName[MAXOBJECTNAME];    // Profile name, "\0" if none
   UCHAR szPassword[MAXOBJECTNAME];       // Password "\0" if none
   UCHAR szOldPassword[MAXOBJECTNAME];    // Old password "\0" if none
   BOOL  bExtrnlPassword;                 // TRUE if external password.
   BOOL  bGrantUser4Rmcmd;                // TRUE gives the rights has this
                                          // user to launch the remote commands
   BOOL  bPartialGrant;                   // TRUE partial grants are defined for this user.
   UINT  iOwnerRmcmdObjects;              // RMCMDOBJETS_NO_OWNER, RMCMDOBJETS_OWNER, RMCMDOBJETS_PARTIAL_OWNER

   BOOL bCreate;
   BOOL  bRemovePwd;                   // TRUE partial grants are defined for this user.
} USERPARAMS, FAR * LPUSERPARAMS;

typedef struct GroupParams {
   UCHAR ObjectName[MAXOBJECTNAME];
   LPCHECKEDOBJECTS lpfirstuserIn;
   LPCHECKEDOBJECTS lpfirstuser;
   LPCHECKEDOBJECTS lpfirstdbIn;
   LPCHECKEDOBJECTS lpfirstdb;

   BOOL bCreate;
   BOOL bRemoveAllUsers; // Only for Drop group;
} GROUPPARAMS, FAR * LPGROUPPARAMS;

typedef struct GroupUserParams {
   UCHAR GroupName [MAXOBJECTNAME];
   UCHAR ObjectName[MAXOBJECTNAME];

   BOOL bCreate;
} GROUPUSERPARAMS, FAR * LPGROUPUSERPARAMS;

typedef struct RoleParams
{
   UCHAR ObjectName [MAXOBJECTNAME];

   BOOL  Privilege         [USERPRIVILEGES];
   BOOL  bDefaultPrivilege [USERPRIVILEGES];   // True if Default privilege[USERxxxPRIV]..
   BOOL  bSecAudit         [USERSECAUDITS];    // TRUE if bSecAudit[USERxxxSECAUD] is checked.
   BOOL  bExtrnlPassword;                      // TRUE if external password.
   BOOL  bCreate;
                                               // ..is checked. All FALSE if NODEFAULT_PRIVILEGES
   LPCHECKEDOBJECTS lpfirstdb;
   LPUCHAR          lpLimitSecLabels;          // Limiting sec labels NULL if ...

   UCHAR szPassword    [MAXOBJECTNAME];        // Password "\0" if none
   //UCHAR szOldPassword [MAXOBJECTNAME];        // Old password "\0" if none

} ROLEPARAMS, FAR * LPROLEPARAMS;

typedef struct IntegrityParams {
   long    number;
   UCHAR   DBName[MAXOBJECTNAME];
   UCHAR   TableName[MAXOBJECTNAME];
   UCHAR   TableOwner[MAXOBJECTNAME];
   LPUCHAR lpSearchConditionIn;
   LPUCHAR lpSearchCondition;
   BOOL     bOnlyText;              // distinguish CREATE from Drag and drop
   LPUCHAR  lpIntegrityText;

   BOOL bCreate;
} INTEGRITYPARAMS, FAR * LPINTEGRITYPARAMS;

typedef struct SchemaParams {
   UCHAR ObjectName[MAXOBJECTNAME];
   LPUCHAR lpSchematext;

   BOOL bCreate;
} SCHEMAPARAMS, FAR * LPSCHEMAPARAMS;

typedef struct SynonymParams {
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR ObjectName[MAXOBJECTNAME];
   UCHAR RelatedObject[MAXOBJECTNAME];
   UCHAR RelatedObjectOwner[MAXOBJECTNAME];

   BOOL bCreate;
} SYNONYMPARAMS, FAR * LPSYNONYMPARAMS;

typedef struct DBEventParams {
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR ObjectName[MAXOBJECTNAME];

   BOOL bCreate;

} DBEVENTPARAMS, FAR * LPDBEVENTPARAMS;

#define SECALARMSSUCFAIL 2
#define SECALARMSUCCESS  0 /* values should be less than SECALARMSSUCFAIL */
#define SECALARMFAILURE  1 /* the defines are used as an index for        */
                           /* bsuccfail in structure SecurityAlarmParams  */

#define SECALARMSTYPES  6
#define SECALARMSEL     0      /* values should be less than SECALARMSTYPES   */
#define SECALARMDEL     1      /* the defines are used as an index for        */
#define SECALARMINS     2      /* bsuccfail in structure SecurityAlarmParams  */
#define SECALARMUPD     3
#define SECALARMCONNECT 4
#define SECALARMDISCONN 5


typedef struct SecurityAlarmParams {
   long SecAlarmId;
   UCHAR ObjectName[MAXOBJECTNAME];       // Security Alaram optional Name
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR PreselectTable [MAXOBJECTNAME];
   UCHAR PreselectTableOwner [MAXOBJECTNAME];

   int  iObjectType;            /* OT_VIRTNODE, OT_DATABASE, or OT_TABLE   */
   int  iAuthIdType;            /* OT_USER, OT_GROUP, OT_ROLE, or OT_PUBLIC*/

   BOOL bDBEvent;
   UCHAR DBEvent[MAXOBJECTNAME];
   LPUCHAR lpDBEventText;

   UCHAR PreselectUser  [MAXOBJECTNAME];
   LPCHECKEDOBJECTS lpfirstTableIn;
   LPCHECKEDOBJECTS lpfirstTable;
   BOOL bsuccfail[SECALARMSSUCFAIL];
   BOOL baccesstype[SECALARMSTYPES];
   LPCHECKEDOBJECTS lpfirstUserIn;
   LPCHECKEDOBJECTS lpfirstUser;

   BOOL bInstallLevel;

} SECURITYALARMPARAMS, FAR * LPSECURITYALARMPARAMS;

typedef struct ProfileParams {               // TO BE FINISHED FOR SEC AUDIT   
   UCHAR    ObjectName[MAXOBJECTNAME];       // Profile Name
   UCHAR    DefaultGroup[MAXOBJECTNAME];     // Group or "\0" if nogroup
   BOOL     Privilege[USERPRIVILEGES];       // True if privilege[USERxxxPRIV]...
                                             // ..is checked. All FALSE if NOPRIVILEGES

   BOOL     bDefaultPrivilege[USERPRIVILEGES]; // True if Default privilege[USERxxxPRIV]..
                                               // ..is checked. All FALSE if NODEFAULT_PRIVILEGES
   UCHAR    ExpireDate[MAXDATELEN];          // expire date or "\0" if none
   LPUCHAR  lpLimitSecLabels;                // Limiting sec labels NULL if ...
                                             // .. nolimiting_security_label)..
                                             // .. MAX length = MAXLIMITSECLBL
   BOOL     bSecAudit[USERSECAUDITS];        // TRUE if bSecAudit[USERxxxSECAUD] is checked. 
   BOOL     bCreate;
   BOOL     bRestrict;                       // For drop profile only

   BOOL bDefProfile;

} PROFILEPARAMS, FAR * LPPROFILEPARAMS;


/* ____________________________________________________________________________
//                                                                             %
// The common grant structure for:                                             %
//                                                                             %
// 1) Database                                                                 %
// 2) Table or View                                                            %
// 3) Database event                                                           %
// 4) Execute procedure                                                        %
// ____________________________________________________________________________%
*/

#define    GRANT_MAX_PRIVILEGES        46
#define    GRANT_MAXDB_PRIVILEGES      17  /* GRANT_ACCESS..GRANT_IDLE_TIME_LIMIT */
#define    GRANT_MAXDB_NO_PRIVILEGES   17  /* GRANT_NO_ACCESS..GRANT_NO_IDLE_TIME_LIMIT */
#define    MAXPRIVNOPRIV               16 

#define    GRANT_ACCESS                 0 // Should start from 0 and ...
#define    GRANT_CREATE_PROCEDURE       1 // incremeted by 1 up to ...
#define    GRANT_CREATE_TABLE           2 // GRANT_MAX_PRIVILEGES minus 1
#define    GRANT_CONNECT_TIME_LIMIT     3
#define    GRANT_DATABASE_ADMIN         4
#define    GRANT_LOCKMOD                5
#define    GRANT_QUERY_IO_LIMIT         6
#define    GRANT_QUERY_ROW_LIMIT        7
#define    GRANT_SELECT_SYSCAT          8
#define    GRANT_SESSION_PRIORITY       9
#define    GRANT_UPDATE_SYSCAT         10
#define    GRANT_TABLE_STATISTICS      11
#define    GRANT_IDLE_TIME_LIMIT       12

#define    GRANT_NO_ACCESS             13
#define    GRANT_NO_CREATE_PROCEDURE   14
#define    GRANT_NO_CREATE_TABLE       15
#define    GRANT_NO_CONNECT_TIME_LIMIT 16
#define    GRANT_NO_DATABASE_ADMIN     17
#define    GRANT_NO_LOCKMOD            18
#define    GRANT_NO_QUERY_IO_LIMIT     19
#define    GRANT_NO_QUERY_ROW_LIMIT    20
#define    GRANT_NO_SELECT_SYSCAT      21
#define    GRANT_NO_SESSION_PRIORITY   22
#define    GRANT_NO_UPDATE_SYSCAT      23
#define    GRANT_NO_TABLE_STATISTICS   24
#define    GRANT_NO_IDLE_TIME_LIMIT    25

#define    GRANT_SELECT                26
#define    GRANT_INSERT                27
#define    GRANT_UPDATE                28
#define    GRANT_DELETE                29
#define    GRANT_COPY_INTO             30
#define    GRANT_COPY_FROM             31
#define    GRANT_REFERENCE             32
#define    GRANT_RAISE                 33
#define    GRANT_REGISTER              34
#define    GRANT_EXECUTE               35

#define    GRANT_QUERY_CPU_LIMIT       36
#define    GRANT_QUERY_PAGE_LIMIT      37
#define    GRANT_QUERY_COST_LIMIT      38
#define    GRANT_NO_QUERY_CPU_LIMIT    39
#define    GRANT_NO_QUERY_PAGE_LIMIT   40
#define    GRANT_NO_QUERY_COST_LIMIT   41

#define    GRANT_CREATE_SEQUENCE       42
#define    GRANT_NO_CREATE_SEQUENCE    43

#define    GRANT_NEXT_SEQUENCE         44
#define    GRANT_ALL                   45

typedef struct tagPrivNoPriv
{
   UINT Priv;
   UINT NoPriv;
}CorrespondTable;
static CorrespondTable PrivNoPriv [MAXPRIVNOPRIV] = /* 16 actually */
{
   {GRANT_ACCESS            , GRANT_NO_ACCESS             },    
   {GRANT_CREATE_PROCEDURE  , GRANT_NO_CREATE_PROCEDURE   },
   {GRANT_CREATE_TABLE      , GRANT_NO_CREATE_TABLE       },
   {GRANT_CONNECT_TIME_LIMIT, GRANT_NO_CONNECT_TIME_LIMIT },
   {GRANT_DATABASE_ADMIN    , GRANT_NO_DATABASE_ADMIN     },
   {GRANT_LOCKMOD           , GRANT_NO_LOCKMOD            },
   {GRANT_QUERY_IO_LIMIT    , GRANT_NO_QUERY_IO_LIMIT     },
   {GRANT_QUERY_ROW_LIMIT   , GRANT_NO_QUERY_ROW_LIMIT    },
   {GRANT_SELECT_SYSCAT     , GRANT_NO_SELECT_SYSCAT      },
   {GRANT_SESSION_PRIORITY  , GRANT_NO_SESSION_PRIORITY   },
   {GRANT_UPDATE_SYSCAT     , GRANT_NO_UPDATE_SYSCAT      },
   {GRANT_TABLE_STATISTICS  , GRANT_NO_TABLE_STATISTICS   },
   {GRANT_IDLE_TIME_LIMIT   , GRANT_NO_IDLE_TIME_LIMIT    },
   {GRANT_QUERY_CPU_LIMIT   , GRANT_NO_QUERY_CPU_LIMIT    },
   {GRANT_QUERY_PAGE_LIMIT  , GRANT_NO_QUERY_PAGE_LIMIT   },
   {GRANT_QUERY_COST_LIMIT  , GRANT_NO_QUERY_COST_LIMIT   }
};


typedef    struct tagPRIVILEGE_IDxSTRING
{
   int     id;
   char*   str;
} PRIVILEGE_IDxSTRING;

typedef    struct tagIDandSTRING
{
   int     id;
   UINT    ids;
   LPUCHAR sql;
   BOOL    bRelatedValue;
} IDandString;

int    GetIdentifier (const char* str);
BOOL   HastPrivilegeRelatedValue (int ConstDefine);
char*  GetPrivilegeString (int ConstDefine);
char*  GetPrivilegeString2(int ConstDefine);
BOOL   HasPrivilegeRelatedValue(int ConstDefine);
void   PrivilegeString_INIT ();
void   PrivilegeString_DONE ();

typedef struct GrantParams
{
   UCHAR   DBName      [MAXOBJECTNAME];
   int     ObjectType;
   int     GranteeType;
   UCHAR   PreselectGrantee [MAXOBJECTNAME];
   UCHAR   PreselectObject  [MAXOBJECTNAME];
   UCHAR   PreselectObjectOwner  [MAXOBJECTNAME];
   BOOL    PreselectPrivileges [GRANT_MAX_PRIVILEGES];

   BOOL    grant_option;                      /* Not use for database, proc */
   BOOL    Privileges  [GRANT_MAX_PRIVILEGES];
   int     PrivValues  [GRANT_MAX_PRIVILEGES];

   LPOBJECTLIST lpgrantee;                     /* Commun for all grant types */
   LPOBJECTLIST lpobject;
   LPOBJECTLIST lpcolumn;

   BOOL bInstallLevel;

} GRANTPARAMS, FAR * LPGRANTPARAMS;

typedef struct RevokeParams
{
   UCHAR   DBName      [MAXOBJECTNAME];
   int     ObjectType;                       /* OT_TABLE ..... */
   int     GranteeType;                      /* OT_USER, OT_GROUP, OT_ROLE, OT_PUBLIC */
   UCHAR   PreselectGrantee [MAXOBJECTNAME];
   UCHAR   PreselectObject  [MAXOBJECTNAME];
   UCHAR   PreselectObjectOwner [MAXOBJECTNAME];
   BOOL    PreselectPrivileges [GRANT_MAX_PRIVILEGES];

   BOOL    grant_option;                     /* TRUE revoke grant option, FALSE privilege */
   BOOL    Privileges [GRANT_MAX_PRIVILEGES];/* the privilege to be revoked */
   BOOL    cascade;                          /* TRUE if cascade, FALSE means restrict */
                                             /* cascade/restrict is meaningless if    */
                                             /* ObjectType = OT_DATABASE              */
   LPOBJECTLIST lpgrantee;                   /* if grantee=PUBLIC, only PUBLIC in list */ 
   LPOBJECTLIST lpobject;
   LPOBJECTLIST lpcolumn;

   BOOL bInstallLevel;

} REVOKEPARAMS, FAR * LPREVOKEPARAMS;

typedef struct RuleParams {
   UCHAR   DBName[MAXOBJECTNAME];
   UCHAR   TableName[MAXOBJECTNAME];
   UCHAR   TableOwner[MAXOBJECTNAME];
   UCHAR   RuleName[MAXOBJECTNAME];
   UCHAR   RuleOwner[MAXOBJECTNAME];
   LPUCHAR lpTableCondition;
   LPUCHAR lpProcedure;
   BOOL     bOnlyText;              // distinguish CREATE from Drag and drop
   LPUCHAR  lpRuleText;
   BOOL    bCreate;   
} RULEPARAMS, FAR * LPRULEPARAMS;

typedef struct ViewParams {
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR objectname [MAXOBJECTNAME];
   UCHAR szSchema   [MAXOBJECTNAME];
   //
   // Need a list of string to store the column name. To be defined
   //

   LPUCHAR lpSelectStatement;
   BOOL    WithCheckOption;
   BOOL     bOnlyText;              // distinguish CREATE from Drag and drop
   LPUCHAR  lpViewText;
   BOOL    bCreate;
} VIEWPARAMS, FAR * LPVIEWPARAMS;

//////////////////////////////////////////////////////////////////////////////
//
// Create Index
//

typedef struct IdxAttributes
{
   BOOL bKey;                    
   BOOL bDescending;               // If heapsort TRUE means descending.
   int nPrimary;                    // > 0 if primary key column
} IDXATTR, FAR * LPIDXATTR;

typedef struct IdxCols
{
	UCHAR szColName[MAXOBJECTNAME];
   IDXATTR attr;
} IDXCOLS, FAR * LPIDXCOLS;

#define IDX_STORAGE_STRUCT_UNKNOWN 0
#define IDX_BTREE 	1
#define IDX_ISAM  	2
#define IDX_HASH  	3
#define IDX_HEAP  	    4
#define IDX_HEAPSORT    5
#define IDX_RTREE       6       // Index structure for spatial datatype (28-01-1997)
#define IDX_VW		7	//VectorWise table structure

#define IDX_UNIQUE_UNKNOWN       0
#define IDX_UNIQUE_NO            1
#define IDX_UNIQUE_ROW           2
#define IDX_UNIQUE_STATEMENT     3
// TO BE FINISHED the followoing structure should not be the same as index.
// typedef struct _StorageParams {   // TO BE FINISHED : temporary for MODIFY (see Francois)
//   UCHAR DBName[MAXOBJECTNAME];
//   UCHAR TableName[MAXOBJECTNAME];
//	UCHAR objectname[MAXOBJECTNAME];
//   UCHAR szSchema[MAXOBJECTNAME];
//   int nModifyAction;            // See actions defines above
//	int nStructure;
//   int nUnique;
//	int nFillFactor;
//	int nMinPages;
//	int nMaxPages;
//	int nLeafFill;
//	int nNonLeafFill;
//   BOOL bPersistence;
//	BOOL bKeyCompression;
//	BOOL bDataCompression;
//   long lAllocation;
//   long lExtend;
//} STORAGEPARAMS, FAR * LPSTORAGEPARAMS;

#define MODIFYACTION_RELOC       0  // Action to be undetaken
#define MODIFYACTION_REORG       1
#define MODIFYACTION_TRUNC       2
#define MODIFYACTION_MERGE       3
#define MODIFYACTION_ADDPAGE     4
#define MODIFYACTION_CHGSTORAGE  5
#define MODIFYACTION_READONLY    6
#define MODIFYACTION_NOREADONLY  7

#define MODIFYACTION_LOGCONSIST         8
#define MODIFYACTION_LOGINCONSIST       9
#define MODIFYACTION_PHYSCONSIST        10
#define MODIFYACTION_PHYSINCONSIST      11
#define MODIFYACTION_RECOVERYALLOWED    12
#define MODIFYACTION_RECOVERYDISALLOWED 13

#define TABLE_PHYS_INCONSISTENT         0x00000200L
#define TABLE_LOG_INCONSISTENT          0x00000400L
#define TABLE_RECOVERY_DISALLOWED       0x00000800L

typedef struct IndexParams {
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR TableName[MAXOBJECTNAME];
   UCHAR TableOwner[MAXOBJECTNAME];
   UCHAR objectname[MAXOBJECTNAME];
   UCHAR objectowner[MAXOBJECTNAME];
   UCHAR szSchema[MAXOBJECTNAME]; // TO BE FINISHED SHOULD BE REMOVED
   int nModifyAction;            // See actions defines above
   int nObjectType;
   int nStructure;
   int nFillFactor;
   LONG nMinPages;             // UK.S 25-11-96: Modify from int to LONG
   LONG nMaxPages;             // UK.S 25-11-96: Modify from int to LONG
   int nLeafFill;
   int nNonLeafFill;
   // issue number 74614 + similar problem for value numrow in properties
   // int nNumRows;
   long nNumRows;
//   STORAGEPARAMS StorageParams;
   LPOBJECTLIST lpLocations;     // linked list of old locations (modify) 
   LPOBJECTLIST lpNewLocations;  // linked list of new locations (modify)  
   long lAllocation;
   long lExtend;
   BOOL bKeyCompression;
   BOOL bDataCompression;
   BOOL bHiDataCompression;
   BOOL bPersistence;
   int nUnique;
   BOOL bCreate;
   LPOBJECTLIST lpidxCols;          // linked list of IDXCOLS
   LPOBJECTLIST lpMaintainCols;     // linked list of IDXCOLS for maintain 
   //
   // OIVERS_20
   LONG uPage_size;                // 0: use the default page size, otherwise must specify.
   TCHAR minX [32];
   TCHAR maxX [32];
   TCHAR minY [32];
   TCHAR maxY [32];

   // For star criterion in GetInfIndex()
   UCHAR szNodeName[MAXOBJECTNAME];

   long lAllocatedPages;   // new field for number of allocated pages
   // For Modify sql statement
   BOOL bReadOnly; // only for a table not used with the index.
   BOOL bPhysInconsistent;   // used for index or table.
   BOOL bLogInconsistent;    // used for index or table.
   BOOL bRecoveryDisallowed; // used for index or table.

   BOOL bConcurrentUpdates; // true generate the SQL statement 'with concurrent_updates'
} INDEXPARAMS, FAR * LPINDEXPARAMS, STORAGEPARAMS, FAR * LPSTORAGEPARAMS;

typedef struct DB_Extensions
{
   char    lname[33];
   long    status;
} DB_EXTENSIONS, FAR * LPDB_EXTENSIONS;

typedef struct DatabaseParams
{
   UCHAR    objectname[MAXOBJECTNAME];
   UCHAR    szOwner[MAXOBJECTNAME];
   UCHAR    szDbDev[MAXOBJECTNAME];
   UCHAR    szChkpDev[MAXOBJECTNAME];
   UCHAR    szJrnlDev[MAXOBJECTNAME];
   UCHAR    szDmpDev[MAXOBJECTNAME];
   UCHAR    szSortDev[MAXOBJECTNAME];
   UCHAR    szCatalogsOIToolsAvailables[MAX_TOOL_PRODUCTS+1];
   BOOL     bPrivate; // TRUE if private database
   LPOBJECTLIST lpDBExtensions; // List of DB_EXTENSIONS
   int      DbType; // database type : DBTYPE_REGULAR, DBTYPE_DISTRIBUTED, DBTYPE_COORDINATOR
   BOOL     bReadOnly;
   BOOL     bUnicodeDBNFD;
   BOOL     bUnicodeDBNFC;
   UCHAR    szCatalogsPageSize[MAXDBNAME];       // Catalogs Page Size
} DATABASEPARAMS, FAR * LPDATABASEPARAMS;

typedef struct ProcedureParams       /* to be defined */
{
   UCHAR    DBName[MAXOBJECTNAME];
	UCHAR    objectname[MAXOBJECTNAME];
	UCHAR    objectowner[MAXOBJECTNAME];
   LPUCHAR  lpProcedureParams;
   LPUCHAR  lpProcedureDeclare;
   LPUCHAR  lpProcedureStatement;
   BOOL     bOnlyText;              // distinguish CREATE from Drag and drop
   LPUCHAR  lpProcedureText;
   BOOL    bCreate;   
   BOOL     bDynamic;   // Ingres Desktop Specific
} PROCEDUREPARAMS, FAR * LPPROCEDUREPARAMS;

typedef struct SequenceParams
{
   UCHAR    szVNode  [MAXOBJECTNAME];   // Virtual node name
   UCHAR    DBName[MAXOBJECTNAME];
   UCHAR    objectname[MAXOBJECTNAME];  // Sequence Name
   UCHAR    objectowner[MAXOBJECTNAME]; // Sequence Owner

   UCHAR    szseq_create[26];
   UCHAR    szseq_modify[26];

   BOOL     bDecimalType;   // TRUE = Decimal Type / FALSE = integer 
   short    iseq_length;
   BOOL     bCycle;
   BOOL     bOrder;
   BOOL     bCreate;

   UCHAR    szMaxValue[50];  // used in create Sequence Dialog Box
   UCHAR    szMinValue[50];
   UCHAR    szStartWith[50];
   UCHAR    szIncrementBy[50];
   UCHAR    szNextValue[50];
   UCHAR    szCacheSize[50];
   UCHAR    szDecimalPrecision[50];
} SEQUENCEPARAMS, FAR * LPSEQUENCEPARAMS;

//////////////////////////////////////////////////////////////////////////////
//
// Create Table
//

enum 
{
	INDEXOPTION_USEDEFAULT = 1,
	INDEXOPTION_NOINDEX,
	INDEXOPTION_BASETABLE_STRUCTURE,
	INDEXOPTION_USEEXISTING_INDEX,
	INDEXOPTION_USER_DEFINED,
	INDEXOPTION_USER_DEFINED_PROPERTY
};

typedef struct tagINDEXOPTION
{
	//
	// bAlter: TRUE if the data is from the DBMS.
	//         ie from the GetDetailInfo.
	BOOL  bAlter;
	//
	// Visual Design
	int  nGenerateMode;   // INDEXOPTION_NOINDEX, ...
	BOOL bDefineProperty;
	
	//
	// These following member should be used to generate index only if 
	// the boolean 'bDefineProperty' is TRUE.:
	TCHAR tchszIndexName [MAXOBJECTNAME];
	TCHAR tchszStructure [16];
	TCHAR tchszFillfactor [16];
	TCHAR tchszMinPage [16];
	TCHAR tchszMaxPage [16];
	TCHAR tchszLeaffill [16];
	TCHAR tchszNonleaffill [16];
	TCHAR tchszAllocation [16];
	TCHAR tchszExtend [16];
	LPSTRINGLIST pListLocation;
} INDEXOPTION, FAR* LPINDEXOPTION;

typedef struct tagPRIMARYKEYSTRUCT
{
	//
	// bAlter: TRUE if the data is from the DBMS.
	//         ie from the GetDetailInfo.
	BOOL  bAlter;

	TCHAR tchszPKeyName [MAXOBJECTNAME]; // Key name (optional)
	//
	// 'pListTableColumn' uses the field extraInfo1 to indicate if column is defined as
	// Nullable or not: (0: nullable, 1: not null)
	LPSTRINGLIST pListTableColumn;       // List of table column (only for visual design)
	LPSTRINGLIST pListKey;               // List of columns (primary key column)
	INDEXOPTION  constraintWithClause;   // Constarint Enhancement (2.6)
	int nMode;                           // (Visual design only: 0: create, 1: alter drop restrict, 2: alter drop cascade)
} PRIMARYKEYSTRUCT, FAR* LPPRIMARYKEYSTRUCT;

typedef struct tagReferenceCols
{
   char szRefingColumn[MAXOBJECTNAME];    // Referencing column
   char szRefedColumn[MAXOBJECTNAME];     // Referenced column
} REFERENCECOLS, FAR * LPREFERENCECOLS;

// Structure describing the referential integrity of the table.  This is a
// list of columns that reference another list of columns, possibly in
// the same table.
// UKS: The system table called iiref_constraint has the field called unique_constraint_name.
// By knowing the unique_constraint_name, we can get all the referenced columns.
// This unique_constraint_name is either the primary key name or unique key name.
typedef struct ReferenceParams
{
	//
	// bAlter: TRUE if the data is from the DBMS.
	//         ie from the GetDetailInfo.
	BOOL  bAlter;

	UCHAR szConstraintName[MAXOBJECTNAME];   // Optional constraint name
	UCHAR szSchema[MAXOBJECTNAME];           // Referenced schema name
	UCHAR szTable[MAXOBJECTNAME];            // Referenced table name
	LPOBJECTLIST lpColumns;                  // List of REFERENCECOLS
	UCHAR szRefConstraint [MAXOBJECTNAME];   // Internal used only. Primary key or unique key constraint name.
	BOOL  bUseDeleteAction;                  // If TRUE, tchszDeleteAction is available
	BOOL  bUseUpdateAction;                  // If TRUE, tchszUpdateAction is available
	TCHAR tchszDeleteAction [16];            // "cascade" | "restrict" | "set null"
	TCHAR tchszUpdateAction [16];            // "cascade" | "restrict" | "set null"
	INDEXOPTION constraintWithClause;        // Constarint Enhancement (2.6)
} REFERENCEPARAMS, FAR * LPREFERENCEPARAMS;

typedef struct tagCONSTRAINTPARAMS {
   char  cConstraintType;
   TCHAR tchszConstraint [MAXOBJECTNAME];
   TCHAR tchszIndex [MAXOBJECTNAME];
   TCHAR tchszSchema[MAXOBJECTNAME];
   LPUCHAR lpText;
} CONSTRAINTPARAMS, far * LPCONSTRAINTPARAMS;

// Structure describing the decimal data type
typedef struct tagDECIMALINFO
{
   int nPrecision;
   int nScale;
} DECIMALINFO, FAR * LPDECIMALINFO;

// Structure describing the length of the various data types
typedef union tagLENINFO
{
   DWORD dwDataLen;        // length of data type
   DECIMALINFO decInfo;    // decimal data type info
} LENINFO, FAR * LPLENINFO;

// Structure describing the columns to be created in the table.
typedef struct ColumnParams
{
   UCHAR szColumn[MAXOBJECTNAME];   // Column name
   UINT uDataType;                  // Data type
   LENINFO lenInfo;                 // Length of data type if applicable
   TCHAR tchszDataType [32];
   TCHAR tchszInternalDataType [32];
   int  nPrimary;                   // > 0 if primary key column
   BOOL bUnique;                    // TRUE if unique data column
   BOOL bNullable;                  // TRUE if column nullable
   BOOL bDefault;                   // TRUE if use default
   LPSTR lpszDefSpec;               // Not NULL if there is a default value
   LPSTR lpszCheck;                 // Not NULL if there is a check constraint
   int  nSecondary;                 // > 0 if secondary key column
   BOOL bHasStatistics;             // TRUE if column has statistics (Ingres only)

   // For table registered as link on a star database
   UCHAR szLdbColumn[MAXOBJECTNAME];   // Column name in the ldb table
   int nKeySequence;                // New (24-Feb-2000): will be set if the column
                                    // is part of primary key or unique key.
   BOOL bSystemMaintained;          // available only with Object_Key or Table_Key Data type.

   struct ColumnParams* next;
} COLUMNPARAMS, FAR * LPCOLUMNPARAMS;


//
// VDBA 2.0 UK.S <<
//

typedef struct tagTLCUNIQUE
{
    //
    // bAlter: TRUE if the data is from the DBMS.
    //         ie from the GetDetailInfo.
    BOOL  bAlter;

    TCHAR tchszConstraint [MAXOBJECTNAME];      // Constraint name;
    BOOL  bTableLevel;                          // TRUE: Table Level (Count (lpcolumnlist) > 1)
    LPSTRINGLIST lpcolumnlist;                  // List of Column to form a constraint
    INDEXOPTION constraintWithClause;          // Constarint Enhancement (2.6)
    struct tagTLCUNIQUE* next;
} TLCUNIQUE, FAR* LPTLCUNIQUE;

/*
typedef struct tagTLCUNIQUEPARAMS
{
    LPOBJECTLIST lpColumns;                     // List of the table columns
    LPTLCUNIQUE  lpctuniquelist;                // List of the Unique Constraint.
    LPTLCUNIQUE  lpolduniquelist;               // List of the Unique Constraint.
    LPSTRINGLIST lpDropObjectList;
    BOOL   bAlter;
} TLCUNIQUEPARAMS, FAR* LPTLCUNIQUEPARAMS;      
*/

typedef struct tagTLCCHECK
{
    //
    // bAlter: TRUE if the data is from the DBMS.
    //         ie from the GetDetailInfo.
    BOOL  bAlter;

    TCHAR  tchszConstraint [MAXOBJECTNAME];     // Constraint name;
    BOOL   bTableLevel;                         // Ignore: Always Table Level.
    LPTSTR lpexpression;                        // String of an expression
    struct tagTLCCHECK* next;
} TLCCHECK, FAR* LPTLCCHECK;

/*
typedef struct tagTLCCHECKPARAM
{
    TCHAR      DBName [MAXOBJECTNAME];
    LPTLCCHECK lpchecklist;
    LPTLCCHECK lpoldchecklist;
    LPSTRINGLIST lpDropObjectList;
    LPSTRINGLIST lpColumns;                     // List of Table columns. For wizard.
    BOOL   bTableLevel;                         // Ignore: Always Table Level.
    BOOL   bAlter;
} TLCCHECKPARAMS, FAR* LPTLCCHECKPARAMS;
*/


//
// VDBA 2.0 UK.S >>
//

// Defines for label granularity element (nLabelGran)
// (only available on B1-enabled CA-Ingres installations)
#define LABEL_GRAN_UNKOWN        0
#define LABEL_GRAN_TABLE         1
#define LABEL_GRAN_ROW           2
#define LABEL_GRAN_SYS_DEFAULTS  3

// Defines for security audit (nSecAudit)
// USE_DEFAULT = use default installation row auditing
#define SECURITY_AUDIT_UNKOWN       0
#define SECURITY_AUDIT_TABLEROW     1
#define SECURITY_AUDIT_TABLENOROW   2
#define SECURITY_AUDIT_ROW          3
#define SECURITY_AUDIT_NOROW        4
#define SECURITY_AUDIT_TABLE        5

////////////////////////////////////////////////////////////////////////////////////////
// For table detail info: filtering in order to minimize the sql requests
typedef enum  tagTableDetailSubset
{
  TABLE_SUBSET_ALL = 0,       // So that it remains as the default value
  TABLE_SUBSET_NOCOLUMNS,
  TABLE_SUBSET_COLUMNSONLY
} TableDetailSubset;


// Structure describing the table to be created.
typedef struct TableParams
{
   UCHAR DBName[MAXOBJECTNAME];           // Database name
   UCHAR szNodeName[MAXOBJECTNAME];       // Virtual node for drag&drop
   UCHAR objectname[MAXOBJECTNAME];       // Table name
   UCHAR szSchema[MAXOBJECTNAME];         // Schema
   LPOBJECTLIST lpColumns;                // List of COLUMNPARAMS
   LPOBJECTLIST lpReferences;             // List of REFERENCEPARAMS (create)
   LPOBJECTLIST lpConstraint;             // List of CONSTRAINTS (properties)    
   LPOBJECTLIST lpLocations;              // List of CHECKEDOBJECTS that
                                          // represents the list of locations
   STORAGEPARAMS StorageParams;
   LPSTR lpszTableCheck;                  // Table level check constraint
   BOOL bJournaling;                      // TRUE if use journaling
   BOOL bDuplicates;                      // Allow duplicate rows
   int nLabelGran;                        // label granularity type
   int nSecAudit;                         // security audit type
   int nbKey;                             // Number of col specified as primary
   UCHAR szSecAuditKey[MAXOBJECTNAME];    // column to use as sec. audit key

   //
   // VDBA V1.5 and V2.0 
   TCHAR tchszPrimaryKeyName [33];        // Internal use for the alter table ... drop ... 
   LPSTRINGLIST lpPrimaryColumns;         // Internal use. List of Column Names that are primary Keys.
   LPSTRINGLIST lpDropObjectList;         // Internal use. List of objects to be dropped. (Columns, constraints ...)
   LPOBJECTLIST lpOldReferences;          // Internal use. Only copy pointer.
   LPTLCUNIQUE  lpOldUnique;              // Internal use. Only copy pointer.
   LPTLCCHECK   lpOldCheck;               // Internal use. Only copy pointer.
   BOOL  bCreateAsSelect;                 // TRUE: If Create table as selecte ... is to be generated
   BOOL	 bCreateVectorWise;
   LPSTR lpColumnHeader;                  // Create table as selecte ... only                  
   LPSTR lpSelectStatement;               // Create table as selecte ... only
   LPTLCUNIQUE lpUnique;                  // Table Level Unique Constraint
   LPTLCCHECK  lpCheck;                   // Table Level Check Constraint
   LONG  uPage_size;                      // 0: use the default page size, otherwise must specify.
   BOOL bCreate;                          // TRUE if creating a table.

   BOOL bPrimKey;                           // TRUE if one col has a rank in prim.key >0
   BOOL bUflag;
   UCHAR UniqueSecIndexName[MAXOBJECTNAME]; // Secondary index name. Empty if none

   // Vdba 2.5
   // new fields for star
   BOOL bDDB;                             // TRUE if DistributedDataBase
   int  hCurNode;                         // current node handle
   char  szCurrentName[MAXOBJECTNAME];    // contents of "name" edit at the time when dialog is called

   // where do we create the table ?
   BOOL  bCoordinator;                    // create on coordinator ?
   UCHAR szLocalNodeName[MAXOBJECTNAME];  
   UCHAR szLocalDBName[MAXOBJECTNAME];
   BOOL  bLocalNodeIsLocal;

   // which name do we use ?
   BOOL  bDefaultName;                    // create with default name ?
   UCHAR szLocalTblName[MAXOBJECTNAME];

   // class for the server
   UCHAR szServerClass[MAXOBJECTNAME];    // INGRES, DB2, etc...

   // Drag and drop from Ingres to a gateway
   // Should we 
   BOOL bGWConvertDataTypes; // TRUE at least one column data type need to be converted.

   // Specific for star table in GetDetailInfo()
   BOOL bRegisteredAsLink;
   char szLdbObjName[33];
   char szLdbObjOwner[33];
   char szLdbDatabase[33];
   char szLdbNode[33];
   char szLdbDbmsType[33];

   //
   // Structure of primary key (with optional constraint enforcement):
   PRIMARYKEYSTRUCT primaryKey;

   // Expiration date
   BOOL bExpireDate;
   long lExpireDate;
   char szExpireDate[33];

   BOOL bReadOnly;

   TableDetailSubset  detailSubset;       // What subset do we query?
   TCHAR              cJournaling;        // Journaling : 'Y', 'C' or 'N' exclusively

} TABLEPARAMS, FAR * LPTABLEPARAMS;

#define TABLEPARAMS_COLUMNS       0x00000001
#define TABLEPARAMS_PRIMARYKEY    0x00000002
#define TABLEPARAMS_UNIQUEKEY     0x00000004
#define TABLEPARAMS_FOREIGNKEY    0x00000008
#define TABLEPARAMS_CHECK         0x00000010
#define TABLEPARAMS_LISTDROPOBJ   0x00000020
#define TABLEPARAMS_ALL           0x0000003F
//
// Duplicate the structure:
// nMember is one or more of TABLEPARAMS_XX above:
BOOL VDBA20xTableStruct_Copy (LPTABLEPARAMS pDest,   LPTABLEPARAMS pSource, UINT nMember);
//
// Free the structure (some or all members):
BOOL VDBA20xTableStruct_Free (LPTABLEPARAMS pStruct, UINT nMember);




/*_____________________________________________________________________________
//                                                                             %
//  Add replication connection structure                                       %
//                                                                             %
//_____________________________________________________________________________%
*/

#define REPL_MIN_NUMBER        1           // Minimum connection number
#define REPL_MIN_SERVER        0           // Minimum server id number
#define REPL_MAX_NUMBER        98          // Maximum connection number
#define REPL_MAX_NUMBER_V11    32767       // Maximum connection number replicator Version 1.1
#define REPL_MAX_SERVER        99          // Maximum server id number
#define MAX_DESCRIPTION_LEN    (80+1)      // Maximum length of a description
#define REPLLOOKUPTABLELEN     24          // Maximum length of lookup table name

/*
// Replication Types  (define in DBA.H)
//
//  REPL_TYPE_FULLPEER         1           // Full peer
//  REPL_TYPE_PROTREADONLY     2           // Protected read-only
//  REPL_TYPE_UNPROTREADONLY   3           // Unprotected read-only
//  REPL_TYPE_GATEWAY          4           // Enterprise access(Gateway target)
*/

typedef struct ReplConnectParams
{
   UCHAR DBName [MAXOBJECTNAME];              // Database name (source);
   char  szDescription [MAX_DESCRIPTION_LEN]; // DBA comment
   int   nNumber;                             // dbno Unique id for connection
   int   nServer;                             // Server id (server role)
   UCHAR szVNode  [MAXOBJECTNAME];            // Virtual node name
   UCHAR szDBName [MAXOBJECTNAME];            // Database name on target machine
   UCHAR szDBOwner[32+1];                     // Owner Database on target machine
   UCHAR nSecurityLevel[2];                   // Security level for replication 1.1
   UCHAR ConfigChanged [25+1];                // Owner Database on target machine
   int   nType;                               // Type of replication to be 
                                              // performed. used for replicator 
                                              // version 1.0
   UCHAR szDBMStype[MAXOBJECTNAME];           // DBMS Type used for replicator 1.1
   UCHAR szShortDescrDBMSType[MAXOBJECTNAME]; // Short description of DBMS type repl 1.1 only
   int   nReplicVersion;                      // Version of replication.
                                              // 1 version 1.0
                                              // 2 version 1.1
   BOOL  bLocalDb;                            // for replicator installation and
                                              // update local database information.
   BOOL  bAlter;                              
   UCHAR szOwner [MAXOBJECTNAME];             // database Owner for remote command 
   // GC_NET_VNODE*  vnodelist;                  // Use to initialize dialog only

} REPLCONNECTPARAMS, FAR * LPREPLCONNECTPARAMS;

// TO BE FINISHED : determine what lookup tables are used for... (Lionel)
typedef struct ReplRegTableParams
{
   UCHAR DBName [MAXOBJECTNAME];             // Dabase name (source)
   UCHAR TableName[MAXOBJECTNAME];           // Registered table 
   UCHAR TableOwner[MAXOBJECTNAME];          // Registered table 
   UCHAR TablesCreated;                      // indicates if tbl created
   UCHAR ColRegistered;                      // indicates col is registered
   UCHAR RulesCreated;                       // indicated rule created
   int   ReplType;                           // replication type
   int   ddRouting;                          // CCDS of record.
   UCHAR RuleLookupTbl[REPLLOOKUPTABLELEN];  // lookup tble
   UCHAR PrioLookupTbl[REPLLOOKUPTABLELEN];  // lookup tble
} REPLREGTABLEPARAMS, FAR * LPREPLREGTABLEPARAMS;

typedef struct ReplDistribParams 
{
   UCHAR DBName [MAXOBJECTNAME];             // Dabase name (source)
   int Local_DBno;                           // database replicator runs on
   int Source_DBno;                          // source database number
   int Target_DBno;                          // target database number
   int ddRouting;                            // CCDS this record.
} REPLDISTRIBPARAMS, FAR * LPREPLDISTRIBPARAMS;
// JFS Begin
//
// Replication CDDS dialog box
//

typedef struct tagDD_CONNECTION
{
 int   dbNo;                             // Database Number
 UCHAR szVNode  [MAXOBJECTNAME];         // Virtual node name
 UCHAR szDBName [MAXOBJECTNAME];         // Database name on target machine
 int   nTypeRepl;                        // replication type :1 Full Peer
                                         // replication type :2 ProtectedRead-only
                                         // replication type :3 Unprotected Read-only
 int   nServerNo;                        // Server id (server role)
 BOOL  bMustRemainFullPeer;              // For subdialog
 BOOL  bIsInCDDS;                        // To remove unused ones at the low level)
 int   CDDSNo;                           // used for some checks in other CDDS
 BOOL  bTypeChosen;                      // internal use in CDDS dialog box
 BOOL  bServerChosen;                    // internal use in CDDS dialog box
 BOOL  bIsTarget;                        // internal use in CDDS dialog box
 int   localdbIfTarget;                  // internal use in CDDS dialog box
 BOOL  bConnectionWithoutPath;           // management for horiz. Partitioning
 BOOL  bElementCanDelete;                // Used for button drop for database in CDDS
} DD_CONNECTION, FAR *LPDD_CONNECTION;

typedef struct tagDD_DISTRIBUTION
{
  int   CDDSNo;   // used for some checks in other CDDS
  int   localdb;  // Local  DB -renamed from local for open ingres
  int   source;   // Source DB
  int   target;   // Target DB
  UCHAR RunNode [MAXOBJECTNAME];  // empty if default is to be used

} DD_DISTRIBUTION, FAR *LPDD_DISTRIBUTION;

typedef struct tagDD_REGISTERED_COLUMNS
{
  UCHAR columnname[MAXOBJECTNAME];// speak for itself
  UINT  key_sequence;             // replicator 1.1 order of the column 
                                  // in the unique key. 0 not part of key
                                  //  
  UINT  column_sequence;          // replicator 1.1 
  UCHAR key_colums[2];            // flag 'K' if column is a key
  UCHAR rep_colums[2];            // flag 'R' if the column is to be replicated
  UINT          cdds;             // identifies CDDS

} DD_REGISTERED_COLUMNS, FAR *LPDD_REGISTERED_COLUMNS;


typedef struct tagREPCDDS
{
  UCHAR DBName       [MAXOBJECTNAME];      // Database name (source)
  UCHAR DBOwnerName  [MAXOBJECTNAME];      // Database owner
  UINT  cdds;                              // CDDS number
  UCHAR szCDDSName   [MAXOBJECTNAME];      // DBMS Type used for replicator 1.1
  UINT  collision_mode;                    // CDDS collision mode repl. 1.1
  UINT  error_mode;                        // CDDS error mode     repl. 1.1
  UINT  LastNumber;                        // Last  number for table_no repl. 1.1
  UINT  ControlDb;                         // repl. 1.1
  BOOL  bAlter;	    		                   // TRUE for Alter CDDS dialog
  int   flag;                              // used by the dialog
  UCHAR szDBMStype   [MAXOBJECTNAME];      // DBMS Type used for replicator 1.1
  LPOBJECTLIST  distribution;              // List of DD_DISTRIBUTION
  LPOBJECTLIST  connections;               // List of DD_CONNECTION
  LPOBJECTLIST  registered_tables;         // List of DD_REGISTERED_TABLES
  int   iReplicType;
} REPCDDS,FAR *LPREPCDDS;

typedef struct tagREPMOBILE
{
  UCHAR DBName       [MAXOBJECTNAME];      // Database name (source)
  UINT  collision_mode;                    // CDDS collision mode repl. 1.05
  UINT  error_mode;                        // CDDS error mode     repl. 1.05
  int   iReplicType;
} REPMOBILE,FAR *LPREPMOBILE;

// JFS End


/*_____________________________________________________________________________
//                                                                             %
// Mail structure                                                              %
//                                                                             %
//_____________________________________________________________________________% 
*/

#define MAX_MAIL_TEXTLEN   81
typedef struct ReplMailParams
{
   UCHAR   DBName      [MAXOBJECTNAME];
   UCHAR   szMailText  [MAX_MAIL_TEXTLEN];

} REPLMAILPARAMS, FAR * LPREPLMAILPARAMS;



#define MAX_LENGTH_COMMENT     1600          // The maximum length for a comment

// Structure to store the comment for column
typedef struct tagCOMMENTCOLUMN
{

   UCHAR szColumnName [MAXOBJECTNAME];
   LPSTR lpszComment;   // Comment

} COMMENTCOLUMN, FAR * LPCOMMENTCOLUMN;

//////////////////////////////////////////////////////////////////////////////
/// sql immediate prototypes
//////////////////////////////////////////////////////////////////////////////

LPCHECKEDOBJECTS GetNextCheckedObject(LPCHECKEDOBJECTS pcheckedobjects);
LPCHECKEDOBJECTS FreeCheckedObjects  (LPCHECKEDOBJECTS pcheckedobjects);
LPCHECKEDOBJECTS AddCheckedObject(LPCHECKEDOBJECTS lc, LPCHECKEDOBJECTS obj);
int BuildSQLSecAlrm(UCHAR **PPstm, LPSECURITYALARMPARAMS secparm);
int BuildSQLCreRole(UCHAR **PPstm, LPROLEPARAMS roleparm);
int BuildSQLCreUsr (UCHAR **PPstm, LPUSERPARAMS usrparm);
int BuildSQLCreProf(UCHAR **PPstm, LPPROFILEPARAMS profparm);
int BuildSQLCreGrp (UCHAR **PPstm, LPGROUPPARAMS grpparm);
int BuildSQLRevoke (UCHAR **PPstm, LPREVOKEPARAMS revokeparm);
int BuildSQLGrant  (UCHAR **PPstm, LPGRANTPARAMS grantparm);
int BuildSQLCreIdx (UCHAR **PPstm, LPINDEXPARAMS idxparm);
int BuildSQLCreProc(UCHAR **PPstm, LPPROCEDUREPARAMS prcparm);
int BuildSQLCreView(UCHAR **PPstm, LPVIEWPARAMS viewparm);
int BuildSQLCreTbl (UCHAR **PPstm, LPTABLEPARAMS tblparm);
int BuildSQLDBGrant(UCHAR **PPstm,int objtype,UCHAR *objname,
                                    LPCHECKEDOBJECTS pchkobj);
int BuildSQLCreSeq (UCHAR **PPstm, LPSEQUENCEPARAMS seqparm);

// JFS Begin
int BuildSQLCDDS        (UCHAR **PPstm, LPREPCDDS lpCdds, LPUCHAR lpNodeName, LPUCHAR lpDBName, int isession);
int BuildSQLCDDSV11     (UCHAR **PPstm, LPREPCDDS lpCdds, LPUCHAR lpNodeName, LPUCHAR lpDBName, int isession);
int BuildSQLDropCDDS    (UCHAR **PPstm, LPREPCDDS lpCdds);
int BuildSQLDropCDDSV11 (UCHAR **PPstm, LPREPCDDS lpCdds);
// JFS End

int DbaToIngType(int iDbaType, int iColLen);
int IngToDbaType(int iIngType, int iColLen);

LPOBJECTLIST RegisterSQLStmt(LPOBJECTLIST lpdescr, HSQL *hsql, LPCTSTR psqlstm);
int FetchSQLStm(LPOBJECTLIST lpFirstdescr, HSQL SQLHinst, ...);
LPOBJECTLIST FreeSqlStm(LPOBJECTLIST lp, HSQL *hsql);
void FreeSqlAreas(LPOBJECTLIST lp);

void * GetFirstSQLDA(void * desc);
// lpsqlda defined as void far *, but used as LPSQLDA in the source
int ExecSQLImm1(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm, long *presultcount);
int ExecSQLImmWithUni(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm);
int ExecSQLImmWithUni1(LPUCHAR lprequest,BOOL bcommit, void far *lpsqldav,
      BOOL (*lpproc)(void *), void *usrparm, long *presultcount);
BOOL UnicodeMngmtNeeded(LPUCHAR lprequest);
BOOL HasUniConstInWhereClause(char *rawstm,char *pStmtWithoutWhereClause);
BOOL ManageDeclareAndOpenCur4Uni(char *cname, char *rawstmt);



int DbaGenReplicObjects(LPUCHAR lpVirtNode, LPUCHAR DBName);
int SqlGetNumberOfHosts(LPOBJECTLIST lp, HSQL hsql);
LPUCHAR SqlGetHostName(LPUCHAR ColName, LPOBJECTLIST lp, HSQL hsql,
                       int iColNumber);          
int SqlLocateFirstRow(LPOBJECTLIST lp, HSQL hsql);
int SqlLocateNextRow(LPOBJECTLIST lp, HSQL hsql);
int SqlGetHostInfos(void **lpValue,LPOBJECTLIST lp, HSQL hsql, int iCol, int *lpilen);
LPUCHAR SQLGetErrorText(long *lplret);
void SQLGetDbmsInfo(long *lBio_cnt, long *lCpu_ms);
BOOL SQLGetPageTypesPerTableTypes(LPUCHAR pNodeName,int * p12ints);

UCHAR *ConcatStr(UCHAR *Lp1, UCHAR *Lp2, int *freebytes);
void StrToLower(LPUCHAR pu);


/****************************************************************** 
   For both Add and Drop object either lpVirtNode or the ...
   va_arg (int session handle) is passed. If a session Handle ...
   has to be passed, the lpVirtNode SHOULD be NULL               
*******************************************************************/
  
int DBAAddObject (LPUCHAR lpVirtNode, int iobjecttype,
          void * lpparameters, ...);
int DBAAddObjectLL (LPUCHAR lpVirtNode, int iobjecttype,
          void * lpparameters, ...);

int DBADropObject(LPUCHAR lpVirtNode, int iobjecttype,
          void * lpparameters, ...);
int DBAUpdMobile(LPUCHAR lpVirtNode, LPREPMOBILE lprepmobile);

int GetMobileInfo(LPREPMOBILE lprepmobile);

int GetDetailInfo(LPUCHAR lpVirtNode,int iobjecttype, void *lpparameters,
                  BOOL bRetainSessForLock, int *ilocsession);
int DBAAlterObject(void *lpObjparams1, void* lpObjParam2,
               int iobjectype, int Sesshdl);
int DBAModifyObject(LPSTORAGEPARAMS lpStorageParams, int Sesshdl);
BOOL AreObjectsIdentical(void *Struct1, void *Struct2, int iobjecttype);
int GenAlterLocation(LPLOCATIONPARAMS prcparm);
int GenAlterGroup(LPGROUPPARAMS OldGrpparm,LPGROUPPARAMS NewGrpparm);
int GenAlterUser(LPUSERPARAMS usrparm);
int GenAlterProfile(LPPROFILEPARAMS profparm);
int GenAlterRole(LPROLEPARAMS roleparm);
int GenAlterSequence(LPSEQUENCEPARAMS sequenceparm);

// Replicator
int GenAlterReplicConnection (LPREPLCONNECTPARAMS OldReplicConnect ,LPREPLCONNECTPARAMS ReplicConnect);
int GenAlterReplicConnectionV11 (LPREPLCONNECTPARAMS OldReplicConnect ,LPREPLCONNECTPARAMS ReplicConnect);
int ReplicConnect( LPREPLCONNECTPARAMS lpaddrepl );
int ReplicConnectV11( LPREPLCONNECTPARAMS lpaddrepl );
int ReplicCDDSV11( LPREPCDDS lpcdds );
int ReplicRetrieveIndexstring( LPDD_REGISTERED_TABLES lpRegTable );
void ReplicDateNow( LPUCHAR dnow );
BOOL AreInstalledSupportTables ( LPUCHAR ProcName);
BOOL AreInstalledRules ( LPUCHAR ProcName);
int ForceFlush( char *table_name,char *table_owner );
int GetReplicatorTableNumber (int hdl ,LPUCHAR lpDbName,LPUCHAR lpTableName,LPUCHAR lpTableOwner, int *nTableNum);
void FreeAttachedPointers (void * pvoid, int iobjecttype);
int GetInfLoc(LPLOCATIONPARAMS locprm);
int GetInfGrp(LPGROUPPARAMS grpprm);
int GetInfUsr(LPUSERPARAMS usrprm);
int GetInfProf(LPPROFILEPARAMS profprm);
int GetInfIntegrity(LPINTEGRITYPARAMS integprm);
int GetInfProcedure(LPPROCEDUREPARAMS procprm);
int GetInfRole(LPROLEPARAMS roleprm);
int GetInfRule(LPRULEPARAMS ruleprm);
int GetInfView(LPVIEWPARAMS viewprm);
int GetInfIndex(LPINDEXPARAMS indexprm);
int GetInfTable(LPTABLEPARAMS tableprm);
int GetInfBase(LPDATABASEPARAMS baseprm);
int GetInfBaseExtend(LPDATABASEPARAMS baseprm);
int GetCatalogPageSize(LPDATABASEPARAMS lpbaseprm);
int GetInfGrantUser4Rmcmd(LPUSERPARAMS lpUsrParam);
int GetInfSequence(LPSEQUENCEPARAMS seqprm);

int ModifyReloc(LPSTORAGEPARAMS lpStorageParams);
int ModifyReorg(LPSTORAGEPARAMS lpStorageParams);
int ModifyTrunc(LPSTORAGEPARAMS lpStorageParams);
int ModifyMerge(LPSTORAGEPARAMS lpStorageParams);
int ModifyAddPages(LPSTORAGEPARAMS lpStorageParams);
int ModifyChgStorage(LPSTORAGEPARAMS lpStorageParams);
int ModifyReadOnly  (LPSTORAGEPARAMS lpStorageParams);
int ModifyNoReadOnly(LPSTORAGEPARAMS lpStorageParams);
int ModifyRecoveryAllowed(LPSTORAGEPARAMS lpStorageParams);
int ModifyRecoveryDisallowed(LPSTORAGEPARAMS lpStorageParams);
int ModifyLogConsistent(LPSTORAGEPARAMS lpStorageParams);
int ModifyLogInconsistent(LPSTORAGEPARAMS lpStorageParams);
int ModifyPhysInconsistent(LPSTORAGEPARAMS lpStorageParams);
int ModifyPhysconsistent(LPSTORAGEPARAMS lpStorageParams);
int ReadInt(int InputFile);
long ReadLong(int InputFile);
int CopyDataAcrossTables(LPUCHAR lpFromNode, LPUCHAR lpFromDb,
                         LPUCHAR lpFromOwner, LPUCHAR lpFromTable,
                         LPUCHAR lpToNode, LPUCHAR lpToDb,
                         LPUCHAR lpToOwner, LPUCHAR lpToTable,
                         BOOL bGWConvertDataTypes,
                         LPTSTR ColList);
int ExportTableData(LPUCHAR lpNode, LPUCHAR lpDb, LPUCHAR lpOwner,
                    LPUCHAR lpTable, LPUCHAR lpFileId, BOOL bGWConvertDataTypes, LPTSTR lpColLst);
int ImportTableData(LPUCHAR lpNode, LPUCHAR lpDb, LPUCHAR lpOwner,
                    LPUCHAR lpTable, LPUCHAR lpFileId);
int DbaPropagateReplicatorCfg(LPUCHAR lpSourceNode,LPUCHAR  lpSourceDb, LPUCHAR lpTargetNode, LPUCHAR lpTargetDb);
int ReplicatorLocalDbInstalled(int hnode,LPUCHAR lpDbName,int ReplicVersion);
LPUCHAR GetColTypeStr(LPCOLUMNPARAMS lpCol);
// bellow : sqlca is defined as void * because esqlca.h must not be included.
#ifndef LIBMON_HEADER
void EnableSqlErrorManagmt();
void DisableSqlErrorManagmt();
#endif
int IsColumnNullable(LPOBJECTLIST lp, HSQL hsql, int iColumnNumber);
LPSQLERRORINFO GetNextSqlError(LPSQLERRORINFO lpSqlError);
LPSQLERRORINFO GetPrevSqlError(LPSQLERRORINFO lpSqlError);
LPSQLERRORINFO GetOldestSqlError();
int UpdCfgChanged(int iReplicType);
void SetCurReplicType(int iRepType);

//
// VDBA20X
//
int VDBA20xGenAlterTable (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS);
int VDBA2xGetTableForeignKeyInfo (LPTABLEPARAMS lpTS);
int VDBA2xGetTableUniqueKeyInfo  (LPTABLEPARAMS lpTS);
int VDBA2xGetTablePrimaryKeyInfo (LPTABLEPARAMS lpTS);

BOOL VDBA20xGetDetailPKeyInfo (LPTABLEPARAMS lpTS, LPTSTR lpVnode);
BOOL VDBA20xGetDetailUKeyInfo (LPTABLEPARAMS lpTS, LPTSTR lpVnode);
BOOL VDBA20xHaveUserDefinedTypes (LPTABLEPARAMS lpTS);

int GetCapabilitiesInfo(LPTABLEPARAMS pTblInfo ,LPGATEWAYCAPABILITIESINFO pGWInfo);
LPTSTR GenerateColList( LPTABLEPARAMS lpSrcTbl );

LPUCHAR lpdefprofiledispstring();

int VDBAGetCommentInfo( LPTSTR lpNodeName,LPTSTR lpDBName, LPTSTR lpObjName,
                        LPTSTR lpObjOwner,LPTSTR *lpObjComment, LPOBJECTLIST lpObjColumn);
int SQLGetComment( LPTSTR lpNode,LPTSTR szDBname, LPTSTR szTableName, LPTSTR szTableOwner,
                   LPTSTR *lpObjectComment, LPOBJECTLIST lpObjColumn);
int VDBA20xGenCommentObject ( LPTSTR lpVirtNode, LPTSTR lpDbName,
                              LPTSTR lpGeneralComment, LPSTRINGLIST lpColComment);

int SQLGetColumnList  ( LPTABLEPARAMS lptbl, LPTSTR lpSqlCommand);
int SQLGetTableStorageStructure(LPTABLEPARAMS lpTbl, LPTSTR lpSqlStruct);

/* 27-Mar-2000 moved here some definitions that were in oidt.h, but used outside the oidt context */

typedef struct tagOIDTOBJECT
{
    TCHAR tchszObject  [MAXOBJECTNAME];   // The name of the object.
    TCHAR tchszCreator [MAXOBJECTNAMENOSCHEMA];  // The creator of the object.
    struct tagOIDTOBJECT* next;
} OIDTOBJECT, FAR* LPOIDTOBJECT;

//
// Structure describing the length of the various data types
//
typedef union tagCOLUMNSIZE
{
    int length;                             // Length of data type
    DECIMALINFO decInfo;                    // Decimal data type info
} COLUMNSIZE, FAR * LPCOLUMNSIZE;


typedef struct tagTABLECOLUMN
{
    TCHAR name      [MAXOBJECTNAMENOSCHEMA];     // Column name
    TCHAR type      [MAXOBJECTNAMENOSCHEMA];     // Column data type. Ex 'CHAR'
    int   nPrimary;                         // From 1 to 16, means primary key sequent.
    int   iType;                            // Equivalent type in numeric representation.
    int   sorted;                           // Only if required. 0: ASC, 1:DESC.
    BOOL  bNullable;                        // TRUE if the column accepts NULL value by default.
    BOOL  bDefault;                         // TRUE means NOT NULL WITH DEFAULT. 
    COLUMNSIZE columnSize;                  // Size of the column.
    TCHAR tchszColExpr [MAXOBJECTNAME];   // Used only by Create Index, when applying the function
                                            // to a column. Ex: @LEFT ("ABCDEF", 2) 
    // specific for Update Statistics
    // nothing at this time
    BOOL bDescending;                       // For desktop

    struct tagTABLECOLUMN* next;
} TABLECOLUMNS, FAR* LPTABLECOLUMNS;


typedef struct tagFOREIGNKEY
{
    TCHAR constraintname [MAXOBJECTNAMENOSCHEMA];// Should limit to 18 characters.
    TCHAR parenttbcreator[MAXOBJECTNAMENOSCHEMA];// Creator of parent table.
    TCHAR parenttbname   [MAXOBJECTNAMENOSCHEMA];// The name of the parent (referenced) table.
    int   deleteMode;                       // 0 if not use. Must be OIDT_DELETERESTRICT, ...
    LPTABLECOLUMNS  lpfkcolumnlist;         // The list of columns to form the foreing key.

    struct tagFOREIGNKEY* next; 
} FOREIGNKEY, FAR* LPFOREIGNKEY;


typedef struct tagTABLESTRUCTURE
{
    // For creating
    TCHAR DBName    [MAXOBJECTNAMENOSCHEMA];     // The name of the database.
    TCHAR creator   [MAXOBJECTNAMENOSCHEMA];     // User who creates table or view
    TCHAR name      [MAXOBJECTNAMENOSCHEMA];     // Table or view name
    LPTABLECOLUMNS  lpcolumnlist;           // List of table columns.             
    LPTABLECOLUMNS  lppkcolumnlist;         // List of primary key columns. Subset of 'lpcolumnlist'.             
    LPFOREIGNKEY    lpfkcolumnlist;         // List of foreign key columns. Subset of 'lpcolumnlist'.
    int  pctfree;                           // A number in [0, 99]. -1 if not use.
    // For detail info
    LONG  rowcount;                         // Number of rows in the table.
    int   colcount;                         // Number of columns in the table.
    // 
    // VDBA V1.5 ->
    BOOL  bCreateAsSelect;                  // TRUE: If Create table as selecte ... is to be generated
    LPSTR lpColumnHeader;                   // Create table as selecte ... only                  
    LPSTR lpSelectStatement;                // Create table as selecte ... only
    // <-
    // Emb 17/06/96 : for drag/drop
	UCHAR szNodeName[MAXOBJECTNAME];        // Virtual node for drag&drop

    BOOL isAlter;                           // TRUE the structure is used to Alter Table.
    struct tagTABLESTRUCTURE* next; 
} TABLESTRUCTURE, FAR* LPTABLESTRUCTURE;

typedef struct tagREFERENCES
{
    TCHAR DBName    [MAXOBJECTNAMENOSCHEMA];     // The name of the database.
    TCHAR tbname    [MAXOBJECTNAMENOSCHEMA];     // The name of the referencing table.
    TCHAR tbcreator [MAXOBJECTNAMENOSCHEMA];     // Creator of the referencing table.
    LPTABLECOLUMNS  lpcolumnlist;           // The list of columns of the referencing table.  
    LPFOREIGNKEY    lpfklist;               // The list of the foreign keys.
} REFERENCES, FAR* LPREFERENCES;

#endif /* LIBMON_HEADER */
#endif   // _DBASET_H
                                     

