/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project  : Visual DBA
**
**    Source   : dbadlg1.h
**
**    Structures and function prototypes of Dialog bog 
**
**  History after 04-May-1999:
**
**   19-Jan-2000 (schph01)
**     Bug #99993
**       Add member 'FileNameModifyByUser' in structure tagTABLExFILE for
**       management of "Ok" and "Cancel" buttons in DlgAuditDBFile dialog box.
**   26-Apr-2001 (noifr01)
**    (RMCMD counterpart of SIR 103299) max lenght of command lines to be
**     executed by rmcmd is now 800 agains 2.6 and later installations. 
**    (old value left when connected to older versions of Ingres)
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 08-Oct-2001 (schph01)
**    SIR 105881, add definition for VerifyNumKeyColumn() function.
** 30-Oct-2001 (schph01)
**    bug 106209 manage the page size option of SYSMOD
** 12-Dec-2001 (noifr01)
**    (sir 99596) removal of obsolete code and resources
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
** 18-Mar-2003 (schph01)
**    (sir 107523) management of sequences
** 09-Apr-2003 (noifr01)
**    (sir 107523) upon cleanup of refresh features for managing sequences,
**    removed obsolete structures and definitions
** 27-Nov-2003 (schph01)
**    (sir 111389) Replace the spin control increasing the checkpoint number by
**    a button displaying the list of existing checkpoints.
** 17-May-2004 (schph01)
**    (SIR 112514) Add definition function VDBA20xAlterTableColumnSyntax()
**    for managing Alter column syntax.
** 23-Nov-2005 (fanra01)
**    Bug 115560
**    Add flags to the ALTERDB structure for controlling command line
**    arguments to the alterdb command.
** 20-Aug-2010 (drivi01)
**    Add a new function to the header DlgAllDBTable.
**    
*/

#ifndef _DBADLG1_H
#define _DBADLG1_H


#include <limits.h>
#include "dba.h"
#include "dbaset.h"
#include "dbatime.h"
#include "settings.h"

#define ZEROINIT(x)		_fmemset((char *)&(x), 0, sizeof((x)))

/*******************************************************************************
 API Summary

 BOOL  WINAPI __export DlgAddReplConnection         (HWND hwndOwner, LPREPLCONNECTPARAMS   lpaddrepl);
 BOOL  WINAPI __export DlgAddUser2Group             (HWND hwndOwner, LPGROUPUSERPARAMS     lpgroupuserparams);
 BOOL  WINAPI __export DlgAlterDB                   (HWND hwndOwner, LPALTERDB             lpalterdb);
 BOOL  WINAPI __export DlgAlterGroup                (HWND hwndOwner, LPGROUPPARAMS         lpgroupparams);
 BOOL  WINAPI __export DlgAlterRole                 (HWND hwndOwner, LPROLEPARAMS          lproleparams );
 BOOL  WINAPI __export DlgAlterUser                 (HWND hwndOwner, LPUSERPARAMS          lpuserparams );
 BOOL  WINAPI __export DlgAuditDB                   (HWND hwndOwner, LPAUDITDBPARAMS       lpauditdb);
 BOOL  WINAPI __export DlgAuditDBFile               (HWND hwndOwner, LPAUDITDBFPARAMS      lpauditdbfparams);
 BOOL  WINAPI __export DlgAuditDBTable              (HWND hwndOwner, LPAUDITDBTPARAMS      lpauditdbtparams);
 BOOL  WINAPI __export DlgAllDBTable              (HWND hwndOwner, LPAUDITDBTPARAMS      lpauditdbtparams);
 BOOL  WINAPI __export DlgCDDS                      (HWND hwndOwner, LPREPCDDS             lpcdds);
 BOOL  WINAPI __export DlgCDDSv11                   (HWND hwndOwner, LPREPCDDS             lpcdds);
 BOOL  WINAPI __export DlgConfSvr                   (HWND hwndOwner);
 BOOL  WINAPI __export DlgReplServer                (HWND hwndOwner, LPDD_CONNECTION       lpConn);
 BOOL  WINAPI __export DlgMobileParam               (HWND hwndOwner, LPREPMOBILE lpmobileparam);
 BOOL  WINAPI __export DlgChangeStorageStructure    (HWND hwndOwner, LPSTORAGEPARAMS       lpstorage);
 BOOL  WINAPI __export DlgCheckpoint                (HWND hwndOwner, LPCHKPT               lpchk);
 BOOL  WINAPI __export DlgCopyDB                    (HWND hwndOwner, LPCOPYDB              lpcopydb);
 BOOL  WINAPI __export DlgCreateDB                  (HWND hwndOwner, LPCREATEDB            lpcreatedb);
 BOOL  WINAPI __export DlgCreateDBevent             (HWND hwndOwner, LPDBEVENTPARAMS       lpdbeventparams);
 BOOL  WINAPI __export DlgCreateGroup               (HWND hwndOwner, LPGROUPPARAMS         lpgroupparams);
 BOOL  WINAPI __export DlgCreateIndex               (HWND hwndOwner, LPINDEXPARAMS         lpidx);
 BOOL  WINAPI __export DlgCreateIntegrity           (HWND hwndOwner, LPINTEGRITYPARAMS     lpintegrityparams);
 BOOL  WINAPI __export DlgCreateLocation            (HWND hwndOwner, LPLOCATIONPARAMS      lplocationparams);
 BOOL  WINAPI __export DlgCreateProcedure           (HWND hwndOwner, LPPROCEDUREPARAMS     lpprocedureparams);
 BOOL  WINAPI __export DlgCreateProfile             (HWND hwndOwner, LPPROFILEPARAMS       lpprofile);
 BOOL  WINAPI __export DlgCreateRole                (HWND hwndOwner, LPROLEPARAMS          lproleparams);
 BOOL  WINAPI __export DlgCreateRule                (HWND hwndOwner, LPRULEPARAMS          lpruleparams);
 BOOL  WINAPI __export DlgCreateSecurityAlarm       (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
 BOOL  WINAPI __export DlgCreateSecurityAlarm2      (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
 BOOL  WINAPI __export DlgCreateSynonym             (HWND hwndOwner, LPSYNONYMPARAMS       lpsynonymparams);
 BOOL  WINAPI __export DlgCreateSynonymOnObject     (HWND hwndOwner, LPSYNONYMPARAMS       lpsynonymparams);
 BOOL  WINAPI __export DlgCreateTable               (HWND hwndOwner, LPTABLEPARAMS         lpcreate);
 BOOL  WINAPI __export DlgCreateUser                (HWND hwndOwner, LPUSERPARAMS          lpuserparams);
 BOOL  WINAPI __export DlgCreateView                (HWND hwndOwner, LPVIEWPARAMS          lpviewparams);
 BOOL  WINAPI __export DlgDisplay                   (HWND hwndOwner, LPDISPLAYPARAMS       lpdisplay);
 BOOL  WINAPI __export DlgDisplayStatistics         (HWND hwndOwner, LPSTATDUMPPARAMS      lpstatdump);
 BOOL  WINAPI __export DlgFind                      (HWND hwndOwner, LPTOOLSFIND           lpToolsFind);
 BOOL  WINAPI __export DlgGetDBLocInfo              (HWND hwndOwner, LPCREATEDB            lpcreatedb);
 BOOL  WINAPI __export DlgGnrefDBevent              (HWND hwndOwner, LPGRANTPARAMS         lpgrant);
 BOOL  WINAPI __export DlgGnrefProcedure            (HWND hwndOwner, LPGRANTPARAMS         lpgrant);
 BOOL  WINAPI __export DlgGnrefTable                (HWND hwndOwner, LPGRANTPARAMS         lpgrant);
 BOOL  WINAPI __export DlgGrantDatabase             (HWND hwndOwner, LPGRANTPARAMS         lpgrantparams);
 BOOL  WINAPI __export DlgGrantDBevent              (HWND hwndOwner, LPGRANTPARAMS         lpgrantparams);
 BOOL  WINAPI __export DlgGrantProcedure            (HWND hwndOwner, LPGRANTPARAMS         lpgrantparams);
 BOOL  WINAPI __export DlgGrantRole2User            (HWND hwndOwner, LPGRANTPARAMS         lpgrantparams);
 BOOL  WINAPI __export DlgGrantTable                (HWND hwndOwner, LPGRANTPARAMS         lpgrantparams);
 BOOL  WINAPI __export DlgLocate                    (HWND hwndOwner, LPLOCATEPARAMS        lplocate);
 BOOL  WINAPI __export DlgLocationSelection         (HWND hwndOwner, LPSTORAGEPARAMS       lpstructure);
 BOOL  WINAPI __export DlgMail                      (HWND hwndOwner, LPREPLMAILPARAMS      lpmail);
 BOOL  WINAPI __export DlgModifyStructure           (HWND hwndOwner, LPSTORAGEPARAMS       lpstructure);
 BOOL  WINAPI __export DlgOpenFile                  (HWND hwndOwner, LPTOOLSOPENFILE       lpToolsOpen);
 BOOL  WINAPI __export DlgOptimizeDB                (HWND hwndOwner, LPOPTIMIZEDBPARAMS    lpoptimizedb);
 BOOL  WINAPI __export DlgPropagate                 (HWND hwndOwner, LPPROPAGATE           lpPropagate); 
 BOOL  WINAPI __export DlgReconciler                (HWND hwndOwner, LPRECONCILER          lpReconciler);
 BOOL  WINAPI __export DlgReferences                (HWND hwndOwner, LPTABLEPARAMS         lptable);
 ULONG WINAPI __export DlgRefresh2                  (HWND hwndOwner, BOOL bCanRefreshCurrent);
 BOOL  WINAPI __export DlgRefSecurity               (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
 BOOL  WINAPI __export DlgRelocate                  (HWND hwndOwner, LPSTORAGEPARAMS       lpstructure);
 BOOL  WINAPI __export DlgRevokeOnDatabase          (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRevokeOnTable             (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRollForward               (HWND hwndOwner, LPROLLFWDPARAMS       lprollfwd);
 BOOL  WINAPI __export DlgRvkgdb                    (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRvkgdbe                   (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRvkgproc                  (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRvkgrol                   (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRvkgtab                   (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgRvkgview                  (HWND hwndOwner, LPREVOKEPARAMS        lprevoke);
 BOOL  WINAPI __export DlgSaveFile                  (HWND hwndOwner, LPTOOLSSAVEFILE       lpToolsSave);
 BOOL  WINAPI __export DlgServerDisconnect          (HWND hwndOwner, LPSERVERDIS           lpsvrdis);
 BOOL  WINAPI __export DlgSession                   (HWND hwndOwner, LPSESSIONPARAMS       lpsession);
 BOOL  WINAPI __export DlgSpaceCalc                 (HWND hwndOwner);
 BOOL  WINAPI __export DlgSpecifyColumns            (HWND hwndOwner, LPSPECIFYCOLPARAMS    lpspecifycolparams);
 BOOL  WINAPI __export DlgSpecifyKey                (HWND hwndOwner, LPINDEXPARAMS         lpidx);
 BOOL  WINAPI __export DlgSQLTestPreferences        (HWND hwndOwner, LPSQLTESTPREF         lptestpref);
 BOOL  WINAPI __export DlgSysMod                    (HWND hwndOwner, LPSYSMOD              lpsysmod);
 BOOL  WINAPI __export DlgTableStructure            (HWND hwndOwner, LPTABLEPARAMS         lptable);
 BOOL  WINAPI __export DlgUnLoadDB                  (HWND hwndOwner, LPCOPYDB              lpcopydb);
 BOOL  WINAPI __export DlgVerifyDB                  (HWND hwndOwner, LPVERIFYDBPARAMS      lpverifydb);
 BOOL  WINAPI __export DlgVnodeSelection            (HWND hwndOwner, LPVNODESEL            lpvnodesel);

 ******************************************************************************/





// *************************************************************************
// *************************************************************************
// Limit defines

#define MAX_DBNAME_LEN         24          // Maximum length for a database name
#define MAX_USERNAME_LEN       24          // Maximum length for a user name
#define MAX_GROUPNAME_LEN      24          // Maximum length for a group name
#define MAX_DBLOCNAME_LEN      24          // Maximum length for a database location name
#define MAX_SERVERNAME_LEN     24          // Maximum length for a database server name
#define MAX_PATH_LEN           260         // Maximum length allowed for a file path
#define MAX_FINDTEXT_LEN       256         // Maximum length of a search string in the 'Find' dialog
#define MAX_VNODE_LEN          24          // Maximum length of a Vnode name
#define MAX_PASSWORD_LEN       24          // Maximum length of a password
#define MAX_LISTENADDR_LEN     24          // Maximum length of a listen address
#define MAX_DEVICENAME_LEN     24          // Maximum length of a device name
#define MAX_INDEXNAME_LEN      24          // Maximum length of an index name
#define MAX_COLNAME_LEN        24          // Maximum length of a tables column name
// Added Emb
#define PASSWORD_LEN           32          // Maximum length of password for env. save
// PS
#define MAX_DIRECTORY_LEN      65          // Maximun length directory in structure copydb
#define MAX_LINE_COMMAND       (GetOIVers() >= OIVERS_26 ? 800 : 255 ) // Maximun length remote command line.
#define MAX_RMCMD_BUFSIZE      (800 + 1)   // max of 800 (2.6 and later) and 255 (older versions)

/* ----------------------------------------------------------------------------
// Structure used to manage the context help stack                            &
//                                                                            &
*/
typedef struct tagSTACKOBJECT
{
   UINT Id;                                // Dialog box identifier (context number for topic of help)
   struct tagSTACKOBJECT* next;
} STACKOBJECT, FAR * LPSTACKOBJECT;
extern LPSTACKOBJECT  lpHelpStack;         // Stack of active dlg box.


/* ----------------------------------------------------------------------------
// Structure for The Propagate Dialog box  (src propagat.c)                   &
//                                                                            &
*/
typedef struct tagPROPAGATE
{
   char DBName     [MAXOBJECTNAME];        // Name of database
   char TargetNode [MAXOBJECTNAME];        // Name of vnode
   char TargetDB   [MAXOBJECTNAME];        // Name of target database
   int  iReplicType;
} PROPAGATE , FAR * LPPROPAGATE;


/* ----------------------------------------------------------------------------
// Structure for The Reconciler Dialog box  (src reconcil.c)                  &
//                                                                            &
*/
typedef struct tagRECONCILER
{
   char DBName     [MAXOBJECTNAME];        // Name of database
   char DbaName    [MAXOBJECTNAME];        // Name of the DBA
   char CddsNo     [MAXOBJECTNAME];        // CDDS number
   char StarTime   [MAXOBJECTNAME];        // Start Time
   char TarGetDbNumber[MAXOBJECTNAME];     // Target Database Number
} RECONCILER, FAR * LPRECONCILER;


/* ----------------------------------------------------------------------------
// Structure for Auditdb Dialog box                                           &
//                                                                            &
*/
typedef struct tagAUDITDB
{
   UCHAR   DBName [MAXOBJECTNAME];         // Database name
   LPUCHAR cmdString;                      // SQL command

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   char szDisplayTableName[MAXOBJECTNAME];
   char szCurrentTableName[MAXOBJECTNAME];

} AUDITDBPARAMS, FAR * LPAUDITDBPARAMS;


/* ----------------------------------------------------------------------------
// Structure for Specifying table Dialog box                                  &
//                                                                            &
*/                                                                            
typedef struct tagAUDITDBT
{
   UCHAR           DBName [MAXOBJECTNAME]; // Database name
   BOOL            bRefuseTblWithDupName;
   LPOBJECTLIST    lpTable;                // List of specified tables
} AUDITDBTPARAMS, FAR * LPAUDITDBTPARAMS;


/* ----------------------------------------------------------------------------
// Structure for Specifing file Dialog box                                    &
//                                                                            &
*/                                                                            
typedef struct tagTABLExFILE               // For each table, there's a file
{                                          // associated.
   UCHAR  TableName [MAXOBJECTNAME];
   UCHAR  FileName  [2*MAXOBJECTNAME];
   UCHAR  FileNameModifyByUser  [2*MAXOBJECTNAME];
   struct tagTABLExFILE* next;

} TABLExFILE, FAR * LPTABLExFILE;

typedef struct tagAUDITDBF
{
   UCHAR           DBName  [MAXOBJECTNAME];// Database name
   LPTABLExFILE    lpTableAndFile;         // List of Couple (Table, File)
} AUDITDBFPARAMS, FAR * LPAUDITDBFPARAMS;


/* ----------------------------------------------------------------------------
// Structure for Optimizedb  Dialog box                                       &
//                                                                            &
*/                                                                            
typedef struct tagOPIMIZEDBPARAMS
{
   UCHAR   DBName [MAXOBJECTNAME];         // Database name
   LPUCHAR cmdString;                      // SQL command

   BOOL bSelectTable;                     // for context-sensitive invocation
   BOOL bSelectIndex;                     // for context-sensitive invocation
   BOOL bSelectColumn;                    // for context-sensitive invocation
   UCHAR TableName[MAXOBJECTNAME];        // used only if bSelectTable is TRUE
   UCHAR TableOwner[MAXOBJECTNAME];       // used only if bSelectTable is TRUE
   UCHAR ColumnName[MAXOBJECTNAME];       // used only if bSelectColumn is TRUE

   char szDisplayTableName[MAXOBJECTNAME];
   TCHAR tchszDisplayIndexName[MAXOBJECTNAME];
   TCHAR tchszIndexName[MAXOBJECTNAME];
   int m_nObjectType;
   BOOL bComposite;                       // Preselect the "Composite histogram ..."
} OPTIMIZEDBPARAMS, FAR * LPOPTIMIZEDBPARAMS;


/* ----------------------------------------------------------------------------
// Structure for Specifying Table columns Dialog box                          &
//                                                                            &
*/                                                                            
typedef struct tagTABLExCOLS               // For each table, there's a list
{                                          // of columns.
   UCHAR        TableName [MAXOBJECTNAME]; // Table name.     
   LPOBJECTLIST lpCol;                     // List of columns.
   struct tagTABLExCOLS * next;
}  TABLExCOLS, FAR * LPTABLExCOLS;

typedef struct tagSPECIFYCOLPARAMS
{
   UCHAR        DBName [MAXOBJECTNAME];    // Database name.
   LPTABLExCOLS lpTableAndColumns;         // List of Couple (Table, List(columns))
}  SPECIFYCOLPARAMS, FAR * LPSPECIFYCOLPARAMS;

/* ----------------------------------------------------------------------------
// Structure for the Roll forward Dialog box                                  &
//                                                                            &
*/                                                                            
typedef struct tagROLLFWD
{
   UCHAR   DBName [MAXOBJECTNAME];         // Database name
   LPUCHAR cmdString;                      // SQL command
   // table list management cloned from copydb source Dec 28, 96
   LPOBJECTLIST  lpTable;

   // relocation info
   int           node;
   LPSTRINGLIST  lpOldLocations;
   LPSTRINGLIST  lpNewLocations;

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   char szDisplayTableName[MAXOBJECTNAME];

}ROLLFWDPARAMS, FAR * LPROLLFWDPARAMS;

/* ----------------------------------------------------------------------------
// Structure for the Verifydb Dialog box                                      &
//                                                                            &
*/                                                                            
typedef struct tagVERIFYDB
{
   UCHAR   DBName [MAXOBJECTNAME];         // Database name.
   LPUCHAR cmdString;                      // SQL command

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   char szDisplayTableName[MAXOBJECTNAME];

} VERIFYDBPARAMS, FAR * LPVERIFYDBPARAMS;


/* ----------------------------------------------------------------------------
// Structure for the Generate statistics (statdump) Dialog box                &
//                                                                            &
*/                                                                            
typedef struct tagSTATDUMP
{
   UCHAR   DBName [MAXOBJECTNAME];         // Database name.
   LPUCHAR cmdString;                      // SQL command.

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   BOOL bStartSinceIndex; // TRUE if launch this dialog box since a branch INDEX
   char szDisplayTableName[MAXOBJECTNAME];
   char szCurrentTableName[MAXOBJECTNAME];
   int m_nObjectType;
   TCHAR tchszDisplayIndexName[MAXOBJECTNAME];
   TCHAR tchszCurrentIndexName[MAXOBJECTNAME];
}STATDUMPPARAMS, FAR * LPSTATDUMPPARAMS;


/* ----------------------------------------------------------------------------
// Structure for the Display Dialog box (SQL Option)                          &
//                                                                            &
*/                                                                            
typedef struct tagDISPLAY
{
   UCHAR szString [100];                   // SQL String.

}DISPLAYPARAMS, FAR * LPDISPLAYPARAMS;

/* ----------------------------------------------------------------------------
// Structure for the Locate Dialog box (src Findobj.c)                        &
//                                                                            &
*/                                                                            
typedef struct tagLOCATE
{
   int   ObjectType ;                      // Object type (Table, View, ...)
   UCHAR DBName     [MAXOBJECTNAME];       // Database name.
   UCHAR FindString [MAXOBJECTNAME];       // String to find.
}LOCATEPARAMS, FAR * LPLOCATEPARAMS;

#if 0

typedef struct tagMODIFYSTRUCTURE
{
   int          ObjectType ;
   UCHAR        DBName [MAXOBJECTNAME];
   LPOBJECTLIST lpLocationList;
}MODIFYSTRUCTUREPARAMS, FAR * LPMODIFYSTRUCTUREPARAMS;

#endif


/* ----------------------------------------------------------------------------
// Structure for the Disconnect Dialog box (src svrdis.c)                     &
//                                                                            &
*/                                                                            
typedef struct tagSERVERDIS
{
   char szServerName[MAX_SERVERNAME_LEN];  // Server name to disconnect
} SERVERDIS, FAR * LPSERVERDIS;


/* ----------------------------------------------------------------------------
// Structure for the Open file Dialog box (src openfile.c)                    &
//                                                                            &
*/                                                                            
typedef struct tagTOOLSOPENFILE
{
   char szFile[MAX_PATH_LEN];              // Full path + file name
} TOOLSOPENFILE, FAR * LPTOOLSOPENFILE;


/* ----------------------------------------------------------------------------
// Structure for the Save file Dialog box (src savefile.c)                    &
//                                                                            &
*/                                                                            
typedef struct tagTOOLSSAVEFILE
{
   char szFile     [MAX_PATH_LEN];         // Full path + file name
   char szPassword [PASSWORD_LEN];
   BOOL bPassword;
} TOOLSSAVEFILE, FAR * LPTOOLSSAVEFILE;


/* ----------------------------------------------------------------------------
// Structure for the Find Dialog box (src find.c)                             &
//                                                                            &
*/                                                                            
// Find dialog
// Notification flags
#define FIND_FINDNEXT      0x0001          // The find next button has been hit
#define FIND_WHOLEWORD     0x0002          // The wholeword option is selected
#define FIND_MATCHCASE     0x0004          // The matchcase option is selected
#define FIND_DIALOGTERM    0x0008          // The dialog is terminating

typedef struct tagTOOLSFIND
{
   HWND hdlg;                              // Returns the handle to the created modeless dialog
   LPSTR lpszFindWhat;                     // Input: Pointer to buffer used to hold the FIND WHAT
                                           //      string. If a string is specified when the 
                                           //      dialog box is created, the string will be used
                                           //      to initialise the FIND WHAT edit control.
   WORD wFindWhatLen;                      // Input: Length of the lpszFindWhat buffer.
   UINT uMessage;                          // Returns message to check for.
} TOOLSFIND, FAR * LPTOOLSFIND;


/* ----------------------------------------------------------------------------
// Structure for the System modification (Sysmod) Dialog box (src sysmod.c)   &
//                                                                            &
*/                                                                            
typedef struct tagSYSMOD
{
   char szDBName[MAXOBJECTNAME];           // Name of database
   UINT uProducts;                         // Products to optimise for
   char szProducts[MAXOBJECTNAME*5];       //
   LPOBJECTLIST  szTables;                 // 
   BOOL bWait;                             // Wait for database to quiesce
   long lPageSize;
} SYSMOD, FAR * LPSYSMOD;


/* ----------------------------------------------------------------------------
// Structure for the AlterDB Dialog box (src find.c)                          &
//                                                                            &
*/                                                                            
typedef struct tagALTERDB
{
   UCHAR DBName[MAXOBJECTNAME];
   BOOL bTgtBlock;
   BOOL bJnlBlockSize;
   BOOL bInitBlock;
   long nBlock;
   long nBlockSize;
   long nInitial;
   BOOL bDisableJrnl;
   BOOL bDelOldestCkPt;
   BOOL bVerbose;
} ALTERDB, FAR * LPALTERDB;


/* ----------------------------------------------------------------------------
// Structure for the Copy Database Dialog box (src copydb.c)                 &
//                                                                            &
*/                                                                            
typedef struct tagCOPYDB
{
   UCHAR DBName    [MAXOBJECTNAME];        // Name of database
   UCHAR UserName  [MAXOBJECTNAME];        // Name of user
   UCHAR DirName   [MAX_DIRECTORY_LEN];    // directrory for -d
   UCHAR DirSource [MAX_DIRECTORY_LEN];    // directory for -source
   UCHAR DirDest   [MAX_DIRECTORY_LEN];    // directory for -dest
   BOOL  bPrintable;
   LPOBJECTLIST  lpTable;                  // list of table for copydb only

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   char szDisplayTableName[MAXOBJECTNAME];
   char szCurrentTableName[MAXOBJECTNAME];

} COPYDB, FAR * LPCOPYDB;


/* ----------------------------------------------------------------------------
// Structure for the Check point Dialog box (src copydb.c)                    &
//                                                                            &
*/                                                                            
typedef struct tagCHKPT
{
   UCHAR DBName[MAXOBJECTNAME];
   UCHAR User[MAXOBJECTNAME];
   UCHAR Device[MAXOBJECTNAME];
   BOOL bDelete;
   BOOL bExclusive;
   BOOL bEnable;
   BOOL bDisable;
   BOOL bWait;
   BOOL bVerbose;
   // table list management cloned from copydb source Dec 28, 96
   LPOBJECTLIST  lpTable;

   BOOL bStartSinceTable; // TRUE if launch this dialog box since a branch TABLE
   char szDisplayTableName[MAXOBJECTNAME];

   // #m management
   BOOL bMulti;
   char szMulti[10];    // will contain a string with numeric value

} CHKPT, FAR * LPCHKPT;

#if 0

typedef struct tagTABLESEL
{
   int dummy;                              // To be defined ???!!!
} TABLESEL, FAR * LPTABLESEL;

#endif

// References dialog
// Local Messages

#define LM_ERROR   WM_USER + 2
// wParam: (int) resource id of error message
// lParam: Style of error box to display
#define PostError(hwnd, resid, flags)\
PostMessage(hwnd, LM_ERROR, (WPARAM)resid, (LPARAM)flags)

#define LM_GETEDITMINMAX   WM_USER + 4
// wParam: (HWND) of edit control
// lParam: Pointer to EDITMINMAX field.
// Returns: TRUE if min max found.
#define Edit_GetMinMax(hwndCtl, lpdesc)\
   (BOOL)(DWORD)SendMessage(GetParent(hwndCtl), LM_GETEDITMINMAX, (WPARAM)hwndCtl, (LPARAM)(LPEDITMINMAX)lpdesc)

#define LM_GETEDITSPINCTRL WM_USER + 5
// wParam: (HWND) of edit control
// lParam: unused
// Returns: (HWND) of spin control
#define Edit_GetSpinControl(hwndCtl, lphwnd)\
   SendMessage(GetParent(hwndCtl), LM_GETEDITSPINCTRL, (WPARAM)hwndCtl, (LPARAM)(HWND FAR *)lphwnd)

// Common structure used to form tables describing
// the edit controls on that dialog
typedef struct tagEDITMINMAX
{
   int nEditId;                        // Edit control ID
   int nSpinId;                        // Associated spin control ID
   DWORD dwMin;                        // Minimum value allowed in edit control
   DWORD dwMax;                        // Maximum value allowed in edit control
} EDITMINMAX, FAR * LPEDITMINMAX;


LPSTACKOBJECT StackObject_TOP  (LPSTACKOBJECT L);
LPSTACKOBJECT StackObject_POP  (LPSTACKOBJECT L);
LPSTACKOBJECT StackObject_PUSH (LPSTACKOBJECT L, LPSTACKOBJECT S);
LPSTACKOBJECT StackObject_FREE (LPSTACKOBJECT L);
LPSTACKOBJECT StackObject_INIT (UINT ContextNum);














// *************************************************************************
// *************************************************************************
//  Propagate dialog


BOOL WINAPI __export DlgPropagate (HWND hwndOwner ,LPPROPAGATE lpPropagate); 


// *************************************************************************
// *************************************************************************
//  Reconciler dialog


BOOL WINAPI __export DlgReconciler (HWND hwndOwner, LPRECONCILER lpReconciler);


// *************************************************************************
// *************************************************************************
// Space calculation dialog

BOOL WINAPI __export DlgSpaceCalc (HWND hwndOwner);
/*
	Function:
		Given the relevant information, calculates the number of
		INGRES pages required for a database table.

	Parameters:
		hwndOwner	- Handle of the parent window for the dialog.

	Returns:
		TRUE if successful.
*/




// *************************************************************************
// *************************************************************************
// Create/Alter database dialog

// Specific front end products to create catalogs for.
#define PROD_NONE				0x0000
#define PROD_INGRES			0x0001	// INGRES base products
#define PROD_VISION			0x0002	// VISION
#define PROD_W4GL				0x0004	// Windows/4GL
#define PROD_INGRES_DBD		0x0008	// INGRES/DBD
#define PROD_NOFECLIENTS	0x0010	// Any user interface products
int WINAPI __export DlgCreateUser (HWND hwndOwner, LPUSERPARAMS lpuserparams);
int WINAPI __export DlgCreateLocation (HWND hwndOwner, LPLOCATIONPARAMS lplocationparams);
int WINAPI __export DlgCreateIntegrity (HWND hwndOwner, LPINTEGRITYPARAMS lpintegrityparams);
int WINAPI __export DlgCreateGroup (HWND hwndOwner, LPGROUPPARAMS lpgroupparams);
int WINAPI __export DlgCreateDBevent (HWND hwndOwner, LPDBEVENTPARAMS lpdbeventparams);
int WINAPI __export DlgCreateSecurityAlarm (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
int WINAPI __export DlgCreateSecurityAlarm2 (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
int WINAPI __export DlgCreateRule (HWND hwndOwner, LPRULEPARAMS lpruleparams);
int WINAPI __export DlgCreateSynonym (HWND hwndOwner, LPSYNONYMPARAMS lpsynonymparams);
int WINAPI __export DlgCreateSynonymOnObject (HWND hwndOwner, LPSYNONYMPARAMS lpsynonymparams);
int WINAPI __export DlgCreateRole (HWND hwndOwner, LPROLEPARAMS lproleparams);
int WINAPI __export DlgCreateView (HWND hwndOwner, LPVIEWPARAMS lpviewparams);
int WINAPI __export DlgCreateProcedure (HWND hwndOwner, LPPROCEDUREPARAMS lpprocedureparams);
unsigned long WINAPI __export DlgRefresh2 (HWND hwndOwner, BOOL bCanRefreshCurrent);
int WINAPI __export DlgGrantTable      (HWND hwndOwner, LPGRANTPARAMS lpgrantparams);
int WINAPI __export DlgGrantProcedure  (HWND hwndOwner, LPGRANTPARAMS lpgrantparams);
int WINAPI __export DlgGrantDBevent    (HWND hwndOwner, LPGRANTPARAMS lpgrantparams);
int WINAPI __export DlgGrantDatabase   (HWND hwndOwner, LPGRANTPARAMS lpgrantparams);
int WINAPI __export DlgGrantRole2User  (HWND hwndOwner, LPGRANTPARAMS lpgrantparams);
int WINAPI __export DlgGnrefTable      (HWND hwndOwner, LPGRANTPARAMS lpgrant);
int WINAPI __export DlgGnrefDBevent    (HWND hwndOwner, LPGRANTPARAMS lpgrant);
int WINAPI __export DlgGnrefProcedure  (HWND hwndOwner, LPGRANTPARAMS lpgrant);
int WINAPI __export DlgRevokeOnTable   (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRevokeOnDatabase(HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgtab         (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgview        (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgdb          (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgrol         (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgdbe         (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRvkgproc        (HWND hwndOwner, LPREVOKEPARAMS lprevoke);
int WINAPI __export DlgRefSecurity     (HWND hwndOwner, LPSECURITYALARMPARAMS lpsecurityalarmparams);
int WINAPI __export DlgAddReplConnection (HWND hwndOwner, LPREPLCONNECTPARAMS lpaddrepl);
int WINAPI __export DlgMail (HWND hwndOwner, LPREPLMAILPARAMS lpmail);
// JFS Begin
int WINAPI __export DlgCDDS           (HWND hwndOwner, LPREPCDDS lpcdds);
int WINAPI __export DlgCDDSv11        (HWND hwndOwner, LPREPCDDS lpcdds);
int WINAPI __export DlgConfSvr        (HWND hwndOwner);
int WINAPI __export DlgReplServer     (HWND hwndOwner, LPDD_CONNECTION lpConn);
int WINAPI __export DlgMobileParam    (HWND hwndOwner, LPREPMOBILE lpmobileparam);
// JFS End
int WINAPI __export DlgAlterUser (HWND hwndOwner, LPUSERPARAMS  lpuserparams );
int WINAPI __export DlgAlterGroup(HWND hwndOwner, LPGROUPPARAMS lpgroupparams);
int WINAPI __export DlgAlterRole (HWND hwndOwner, LPROLEPARAMS  lproleparams );
int WINAPI __export DlgAddUser2Group (HWND hwndOwner, LPGROUPUSERPARAMS lpgroupuserparams);
int WINAPI __export DlgAuditDB (HWND hwndOwner, LPAUDITDBPARAMS lpauditdb);
int WINAPI __export DlgAuditDBTable (HWND hwndOwner, LPAUDITDBTPARAMS lpauditdbtparams);
int WINAPI __export DlgAllDBTable (HWND hwndOwner, LPAUDITDBTPARAMS lpauditdbtparams);
int WINAPI __export DlgAuditDBFile (HWND hwndOwner, LPAUDITDBFPARAMS lpauditdbfparams);
int WINAPI __export DlgOptimizeDB (HWND hwndOwner, LPOPTIMIZEDBPARAMS lpoptimizedb);
int WINAPI __export DlgSpecifyColumns (HWND hwndOwner, LPSPECIFYCOLPARAMS lpspecifycolparams);
BOOL WINAPI __export DlgRollForward (HWND hwndOwner, LPROLLFWDPARAMS lprollfwd);
BOOL WINAPI __export DlgRollForwardRelocate (HWND hwndOwner, LPROLLFWDPARAMS lprollfwd);
int WINAPI __export DlgDisplayStatistics (HWND hwndOwner, LPSTATDUMPPARAMS lpstatdump);
int WINAPI __export DlgDisplay (HWND hwndOwner, LPDISPLAYPARAMS lpdisplay);
BOOL WINAPI __export DlgVerifyDB (HWND hwndOwner, LPVERIFYDBPARAMS lpverifydb);
BOOL WINAPI __export DlgLocate (HWND hwndOwner, LPLOCATEPARAMS lplocate);
BOOL WINAPI __export DlgCreateProfile (HWND hwndOwner, LPPROFILEPARAMS lpprofile);
BOOL WINAPI __export DlgModifyStructure        (HWND hwndOwner, LPSTORAGEPARAMS lpstructure);
BOOL WINAPI __export DlgLocationSelection      (HWND hwndOwner, LPSTORAGEPARAMS lpstructure);
BOOL WINAPI __export DlgChangeStorageStructure (HWND hwndOwner, LPSTORAGEPARAMS lpstorage);
BOOL WINAPI __export DlgRelocate               (HWND hwndOwner, LPSTORAGEPARAMS lpstructure);

BOOL WINAPI __export DlgGetDBLocInfo (HWND hwndOwner, LPCREATEDB lpcreatedb);
BOOL WINAPI __export DlgServerDisconnect (HWND hwndOwner, LPSERVERDIS lpsvrdis);
BOOL WINAPI __export DlgOpenFile(HWND hwndOwner, LPTOOLSOPENFILE lpToolsOpen);
BOOL WINAPI __export DlgSaveFile(HWND hwndOwner, LPTOOLSSAVEFILE lpToolsSave);


// *************************************************************************
// *************************************************************************

BOOL WINAPI __export DlgFind (HWND hwndOwner, LPTOOLSFIND lpToolsFind);
/*
	Function:
		Creates a modeless dialog box that can be used to perform search
		operations.

	Parameters:
		hwndOwner		- Handle to the owner window
		lpToolsFind		- Points to structure containing information used to 
							- initialise the dialog. The dialog uses the same
							- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		Since the dialog created is modeless,  the IsDialogMessage function
		should be called in the applications main message loop.

		The application is notified of any action in the dialog via a
		message whose id is returned in the uMessage structure element.
		The application should therefore check for this message in its
		window procedure.

		The wParam parameter of the message contains the notification flags.

		The TOOLSFIND structure that is used to initialise the dialog should
		not be destroyed until the dialog is dismissed.	

		When the application receives the FIND_DIALOGTERM notification, it
		should tidy up any memory associated with the dialog and end its
		check for modeless dialogs in its message loop.
*/




// *************************************************************************
// *************************************************************************

BOOL WINAPI __export DlgSysMod (HWND hwndOwner, LPSYSMOD lpsysmod);


// *************************************************************************
// *************************************************************************
// Alter Database dialog

#define ALTERDB_MIN_BLOCK		0 		// Minimum journal block
#define ALTERDB_MIN_INITIAL	0		  // Minimum initial block
#define ALTERDB_MAX_BLOCK		65536 // Maximum journal block
#define ALTERDB_MAX_INITIAL	65536 // Maximum initial block

/* Size Types */
// ???!!!
#define ALTERDB_SIZE_xxxx


BOOL WINAPI __export DlgAlterDB (HWND hwndOwner, LPALTERDB lpalterdb);
/*
	Function:
		Shows the Alter Database dialog.

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lpalterdb	- Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/



int WINAPI __export DlgSession (HWND hwndOwner, LPSESSIONPARAMS lpsession);

BOOL LoadSessionSettings           (LPSESSIONPARAMS lpsess);
BOOL WriteSessionSettings          (LPSESSIONPARAMS lpsess);
BOOL LoadCurrentSessionSettings    (LPSESSIONPARAMS lpsess);






/*
//
// Structure for Vnode selection:  define in file: GETVNODE.H                                                    
//
*/

//int WINAPI __export DlgVnodeSelection (HWND hwndOwner, LPVNODESEL lpvnodesel);





BOOL WINAPI __export DlgUnLoadDB (HWND hwndOwner, LPCOPYDB lpcopydb);
BOOL WINAPI __export DlgCopyDB (HWND hwndOwner, LPCOPYDB lpcopydb);


// *************************************************************************
// *************************************************************************
BOOL WINAPI __export DlgCheckpoint (HWND hwndOwner, LPCHKPT lpchk);
/*
	Function:
		Shows the Checkpoint dialog.

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lpchk			- Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/



// *************************************************************************
// *************************************************************************
// Create Index dialog

#define IDX_MIN_MINPAGES		1
#define IDX_MAX_MINPAGES		8388607
#define IDX_MIN_MAXPAGES		1
#define IDX_MAX_MAXPAGES		8388607
#define IDX_MIN_LEAFFILL		1
#define IDX_MAX_LEAFFILL		100
#define IDX_MIN_NONLEAFFILL	1
#define IDX_MAX_NONLEAFFILL	100
#define IDX_MIN_FILLFACTOR		1
#define IDX_MAX_FILLFACTOR		100
#define IDX_MIN_ALLOCATION    4
#define IDX_MAX_ALLOCATION    8388607
#define IDX_MIN_EXTEND        1
#define IDX_MAX_EXTEND        8388607

int WINAPI __export DlgCreateIndex (HWND hwndOwner, LPINDEXPARAMS lpidx);
/*
	Function:
		Show the create index dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lpidx			- Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/

// *************************************************************************
// *************************************************************************
// Create Table dialog

int WINAPI __export DlgCreateTable (HWND hwndOwner, LPTABLEPARAMS lpcreate);
/*
	Function:
		Shows the Create Table dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lpcreate 	- Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/

// *************************************************************************
// *************************************************************************
// Table Structure dialog

int WINAPI __export DlgTableStructure (HWND hwndOwner, LPTABLEPARAMS lptable);
/*
	Function:
		Shows the Table Structure dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lptable 	   - Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/


// *************************************************************************
// *************************************************************************
// References dialog

int WINAPI __export DlgReferences (HWND hwndOwner, LPTABLEPARAMS lptable);
/*
	Function:
		Shows the References dialog

	Paramters:
		hwndOwner	- Handle of the parent window for the dialog.
		lptable 	   - Points to structure containing information used to 
						- initialise the dialog. The dialog uses the same
						- structure to return the result of the dialog.

	Returns:
		TRUE if successful.

	Note:
		If invalid parameters are passed in they are reset to a default
		so the dialog can be safely displayed.
*/

// ************************************************************************

HWND GetEditSpinControl    (HWND hwndEdit, LPEDITMINMAX    lplimits);
BOOL ProcessSpinControl    (HWND hwndCtl,  UINT codeNotify,LPEDITMINMAX lpLimits);
BOOL GetEditCtrlMinMaxValue(HWND hwnd,     HWND hwndCtl,   LPEDITMINMAX lpLimits, DWORD FAR * lpdwMin, DWORD FAR * lpdwMax);


LPOBJECTLIST   AddListObject     (LPOBJECTLIST lpList, int cbObjSize);
LPOBJECTLIST   AddListObjectTail (LPOBJECTLIST FAR * lplpList, int cbObjSize);
LPOBJECTLIST   DeleteListObject  (LPOBJECTLIST lpList, LPOBJECTLIST lpDelElement);
void           FreeObjectList    (LPOBJECTLIST lpList);
int            CountListItems    (LPOBJECTLIST lplist);
void   my_itoa (int value, char FAR * lpszBuffer, int radix);
int    my_atoi (char FAR * lpszBuffer);
DWORD  my_atodw (char FAR * lpszBuffer);
void   my_dwtoa (DWORD value, char FAR * lpszBuffer, int radix);
void   GetExactDisplayString (
           const   char * aString,
           const   char* aOwner,
           int     aObjectType,
           LPUCHAR parentstrings [MAXPLEVEL],
           char*   buffExactDispString);

// Added Emb Sep. 23, 95 for critical section in INITDIALOG phase
int WINAPI VdbaDialogBox(HINSTANCE, LPCSTR, HWND, DLGPROC);
int WINAPI VdbaDialogBoxParam(HINSTANCE, LPCSTR, HWND, DLGPROC, LPARAM);



int WINAPI EXPORT CTCheck  (HWND hwndOwner, LPTABLEPARAMS lpTable);
int WINAPI EXPORT CTUnique (HWND hwndOwner, LPTABLEPARAMS lpTable);



UINT StringList_Count            (LPSTRINGLIST list);
LPSTRINGLIST StringList_Add      (LPSTRINGLIST oldList, LPCTSTR lpszString);
LPSTRINGLIST StringList_AddObject(LPSTRINGLIST oldList, LPCTSTR lpszString, LPSTRINGLIST* lpObj);
LPSTRINGLIST StringList_Done     (LPSTRINGLIST oldList);
LPSTRINGLIST StringList_Search   (LPSTRINGLIST oldList, LPCTSTR lpszString);
LPSTRINGLIST StringList_Remove   (LPSTRINGLIST oldList, LPCTSTR lpszString);
LPSTRINGLIST StringList_Copy     (LPSTRINGLIST lpList);
BOOL StringList_Included         (LPSTRINGLIST lpList1, LPSTRINGLIST lpList2);
BOOL StringList_Equal            (LPSTRINGLIST lpList1, LPSTRINGLIST lpList2);

LPSTRINGLIST VDBA20xDropTableColumnSyntax(LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);
LPSTRINGLIST VDBA20xAddTableColumnSyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);
LPSTRINGLIST VDBA20xAlterTableColumnSyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);


LPREFERENCEPARAMS VDBA20xForeignKey_Search(LPOBJECTLIST lpReferences, LPCTSTR lpszConstraint);
LPREFERENCEPARAMS VDBA20xForeignKey_Lookup(LPOBJECTLIST lpReferences, LPREFERENCEPARAMS lpRef);
LPOBJECTLIST VDBA20xTableReferences_Remove(LPOBJECTLIST list, LPCTSTR lpszConstraint);
LPOBJECTLIST VDBA20xTableReferences_Done  (LPOBJECTLIST list);
LPOBJECTLIST VDBA20xForeignKey_Add        (LPOBJECTLIST list, LPREFERENCEPARAMS lpRef);

LPSTRINGLIST VDBA20xAlterForeignKeySyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);
LPSTRINGLIST VDBA20xAlterPrimaryKeySyntax (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);
LPSTRINGLIST VDBA20xAlterUniqueKeySyntax  (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);
LPSTRINGLIST VDBA20xAlterCheckSyntax      (LPTABLEPARAMS lpOLDTS, LPTABLEPARAMS lpNEWTS, int* status);

LPOBJECTLIST   VDBA20xColumnList_Copy (LPOBJECTLIST first);
LPOBJECTLIST   VDBA20xColumnList_Done (LPOBJECTLIST first);
LPCOLUMNPARAMS VDBA20xTableColumn_Search (LPOBJECTLIST lpColumn, LPCTSTR lpszColumn);
LPCOLUMNPARAMS VDBA20xTableColumn_SortedAdd (LPCOLUMNPARAMS first, LPCOLUMNPARAMS obj);
BOOL VDBA20xTableColumn_Included  (LPOBJECTLIST lpCol1, LPOBJECTLIST lpCol2);
BOOL VDBA20xTablePrimaryKey_Equal (LPOBJECTLIST lpCol1, LPOBJECTLIST lpCol2);
BOOL VDBA20xTableForeignKey_Equal (LPOBJECTLIST lpCol1, LPOBJECTLIST lpCol2);
BOOL VDBA20xTable_Equal (LPTABLEPARAMS lpTS1, LPTABLEPARAMS lpTS2);

LPTLCUNIQUE VDBA20xTableUniqueKey_Lookup (LPTLCUNIQUE lpUniques, LPTLCUNIQUE lpUniqueKey);
LPTLCUNIQUE VDBA20xTableUniqueKey_Search (LPTLCUNIQUE lpUniques, LPCTSTR lpszConstraint);
LPTLCUNIQUE VDBA20xTableUniqueKey_Add    (LPTLCUNIQUE lpUniques, LPTLCUNIQUE lpUniqueKey);
LPTLCUNIQUE VDBA20xTableUniqueKey_Remove (LPTLCUNIQUE lpUniques, LPCTSTR lpszConstraint);
LPTLCUNIQUE VDBA20xTableUniqueKey_Done   (LPTLCUNIQUE lpUniques);

LPTLCCHECK VDBA20xTableCheck_Lookup (LPTLCCHECK lpCheck, LPTLCCHECK lpObj);
LPTLCCHECK VDBA20xTableCheck_Search (LPTLCCHECK lpCheck, LPCTSTR lpszConstraint);
LPTLCCHECK VDBA20xTableCheck_Add    (LPTLCCHECK lpCheck, LPTLCCHECK lpObj);
LPTLCCHECK VDBA20xTableCheck_Remove (LPTLCCHECK lpCheck, LPCTSTR lpszConstraint);
LPTLCCHECK VDBA20xTableCheck_Done   (LPTLCCHECK lpCheck);

BOOL WINAPI EXPORT VDBA2xAlterTable (HWND hwndOwner, LPTABLEPARAMS lpTableStructure);


LPOBJECTLIST VDBA20xTableReferences_CopyList (LPOBJECTLIST lpRef);
LPTLCUNIQUE  VDBA20xTableUniqueKey_CopyList  (LPTLCUNIQUE  lpList);
LPTLCCHECK   VDBA20xTableCheck_CopyList      (LPTLCCHECK   lpList);

BOOL IsTheSameOwner(int hNode,char *szTblOwner);
BOOL IsTableNameUnique(int hNode, char *DbName, char *tblName);

BOOL VerifyNumKeyColumn(LPTSTR lpVnode, LPINDEXPARAMS lpIndexPar);

#endif	// _DBADLG1_H
