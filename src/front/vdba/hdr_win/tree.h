/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : tree.h
//   manages the lbtree inside a dom type mdi document
//
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  18-Mar-2003 (schph01)
**   sir 107523 management of sequences
**  20-May-2010 (drivi01)
**   Add OT_VW_TABLE to differentiate Ingres tables from
**   Ingres VectorWise tables for a custom icon.
********************************************************************/

#ifndef __TREE_INCLUDED__
#define __TREE_INCLUDED__

#ifdef _INC_WINDOWSX
    #undef GetNextSibling
#endif

//
// Global structures definitions
//
typedef struct  _sTreeRecord
{
  int   recType;                    // record type
  BOOL  bSubValid;                  // if has sub branch, is sub data valid?
  BOOL  bToBeDeleted;               // marqued as "to be deleted" ?
  UCHAR extra[MAXOBJECTNAME];       // extra info: parenthood of level 1
  UCHAR extra2[MAXOBJECTNAME];      // extra info: parenthood of level 2
  UCHAR extra3[MAXOBJECTNAME];      // extra info: parenthood of level 3
  UCHAR objName[MAXOBJECTNAME];     // Object name (caption may be different)
  UCHAR ownerName[MAXOBJECTNAME];   // Object's owner name
  UCHAR szComplim[MAXOBJECTNAME];   // Complimentary data received (maxio,...)
  UCHAR tableOwner[MAXOBJECTNAME];  // if parent 1 is table: it's schema
  UCHAR wildcardFilter[MAXOBJECTNAME];
  long  complimValue;               // integrity/security number, ...
  int   parentbranch;               // parent branch management

  // Added for fnn request 16/05/97 :
  int displayType;                  // for solved grantees/alarmees

  // Added for star object managements, used by drag-drop
  int parentDbType;                 // parent database type : DBTYPE_REGULAR, DBTYPE_DISTRIBUTED, DBTYPE_COORDINATOR

  // Added Nov 26 for right pane (dom with splitbar)
  BOOL  bPageInfoCreated;
  DWORD m_pPageInfo;      // cast into CuPageInformation on the C++ side

  // Added Sep.9, 98 for old behaviour compatibility with early type solve
  int unsolvedRecType;

} TREERECORD, FAR * LPTREERECORD;

// constants for parentbranch - powers of two since they need to be combined
#define PARENTBRANCH_THERE  1   // has parent branch been created? (yes if 1)
#define PARENTBRANCH_NEEDED 2   // needs to create a parent branch when expanded

// constants
// MAKE SURE THE OT_xxx VALUES DO NOT OVERLAP WITH OT_yyy VALUES IN DBA.H !

// level 0
#define OT_STATIC_DATABASE  201
#define OT_STATIC_PROFILE   202
#define OT_STATIC_USER      203
#define OT_STATIC_GROUP     204
#define OT_STATIC_ROLE      205
#define OT_STATIC_LOCATION  206

#define OT_USER_SUBSTATICS        207
#define OT_GROUP_SUBSTATICS       208
#define OT_ROLE_SUBSTATICS        209
#define OT_REPLICATOR_SUBSTATICS  210
#define OT_ALARM_SUBSTATICS       211

#define OT_STATIC_SYNONYMED       220

// "Solved" types - start from 240
#define OT_GRANTEE_SOLVED_USER     240
#define OT_GRANTEE_SOLVED_GROUP    241
#define OT_GRANTEE_SOLVED_ROLE     242

// level 1, child of database - start from 260
#define OT_DATABASE_SUBSTATICS  260
#define OT_STATIC_TABLE         261
#define OT_STATIC_VIEW          262
#define OT_STATIC_PROCEDURE     263
#define OT_STATIC_SCHEMA        264
#define OT_STATIC_SYNONYM       265
#define OT_STATIC_DBEVENT       266

#define OT_TABLE_SUBSTATICS       267
#define OT_VIEW_SUBSTATICS        268

// continued after other level 1 not to resequence ids

// level 1, child of user
#define OT_STATIC_R_USERSCHEMA  269
#define OT_STATIC_R_USERGROUP   270

#define OT_STATIC_R_SECURITY        271
#define OT_STATIC_R_SEC_SEL_SUCC    273
#define OT_STATIC_R_SEC_SEL_FAIL    274
#define OT_STATIC_R_SEC_DEL_SUCC    276
#define OT_STATIC_R_SEC_DEL_FAIL    277
#define OT_STATIC_R_SEC_INS_SUCC    279
#define OT_STATIC_R_SEC_INS_FAIL    280
#define OT_STATIC_R_SEC_UPD_SUCC    282
#define OT_STATIC_R_SEC_UPD_FAIL    283

#define OT_STATIC_R_GRANT               284     // rights granted
#define OT_STATIC_R_DBEGRANT            285     // grants for dbevents
#define OT_STATIC_R_DBEGRANT_RAISE      286
#define OT_STATIC_R_DBEGRANT_REGISTER   287
#define OT_STATIC_R_PROCGRANT           288     // grants for procedures
#define OT_STATIC_R_PROCGRANT_EXEC      289
#define OT_STATIC_R_DBGRANT             290     // grants for databases
#define OT_STATIC_R_DBGRANT_ACCESY      291
#define OT_STATIC_R_DBGRANT_ACCESN      292
#define OT_STATIC_R_DBGRANT_CREPRY      293
#define OT_STATIC_R_DBGRANT_CREPRN      294
#define OT_STATIC_R_DBGRANT_CRETBY      295
#define OT_STATIC_R_DBGRANT_CRETBN      296
#define OT_STATIC_R_DBGRANT_DBADMY      297
#define OT_STATIC_R_DBGRANT_DBADMN      298
#define OT_STATIC_R_DBGRANT_LKMODY      299
#define OT_STATIC_R_DBGRANT_LKMODN      300
#define OT_STATIC_R_DBGRANT_QRYIOY      301
#define OT_STATIC_R_DBGRANT_QRYION      302
#define OT_STATIC_R_DBGRANT_QRYRWY      303
#define OT_STATIC_R_DBGRANT_QRYRWN      304
#define OT_STATIC_R_DBGRANT_UPDSCY      305
#define OT_STATIC_R_DBGRANT_UPDSCN      306

#define OT_STATIC_R_DBGRANT_SELSCY      307
#define OT_STATIC_R_DBGRANT_SELSCN      308
#define OT_STATIC_R_DBGRANT_CNCTLY      309
#define OT_STATIC_R_DBGRANT_CNCTLN      310
#define OT_STATIC_R_DBGRANT_IDLTLY      311
#define OT_STATIC_R_DBGRANT_IDLTLN      312
#define OT_STATIC_R_DBGRANT_SESPRY      313
#define OT_STATIC_R_DBGRANT_SESPRN      314
#define OT_STATIC_R_DBGRANT_TBLSTY      315
#define OT_STATIC_R_DBGRANT_TBLSTN      316

#define OT_STATIC_R_TABLEGRANT          317     // grants for tables
#define OT_STATIC_R_TABLEGRANT_SEL      318
#define OT_STATIC_R_TABLEGRANT_INS      319
#define OT_STATIC_R_TABLEGRANT_UPD      320
#define OT_STATIC_R_TABLEGRANT_DEL      321
#define OT_STATIC_R_TABLEGRANT_REF      322

#define OT_STATIC_R_TABLEGRANT_CPI      323
#define OT_STATIC_R_TABLEGRANT_CPF      324

// new section for previously undocumented dbgrants
#define OT_STATIC_R_DBGRANT_QRYCPY      325
#define OT_STATIC_R_DBGRANT_QRYCPN      326
#define OT_STATIC_R_DBGRANT_QRYPGY      327
#define OT_STATIC_R_DBGRANT_QRYPGN      328
#define OT_STATIC_R_DBGRANT_QRYCOY      329
#define OT_STATIC_R_DBGRANT_QRYCON      330

#define OT_STATIC_R_TABLEGRANT_ALL      333
#define OT_STATIC_R_VIEWGRANT           334     // grants for views
#define OT_STATIC_R_VIEWGRANT_SEL       335
#define OT_STATIC_R_VIEWGRANT_INS       336
#define OT_STATIC_R_VIEWGRANT_UPD       337
#define OT_STATIC_R_VIEWGRANT_DEL       338
#define OT_STATIC_R_ROLEGRANT           339     // grants for roles

// level 1, child of group
#define OT_STATIC_GROUPUSER             341

// level 1, children of location
#define OT_STATIC_R_LOCATIONTABLE       342

// level 1, children of users/groups/roles
#define OTR_GRANTS_SUBSTATICS           343
#define OT_STATIC_ROLEGRANT_USER        344

// level 1, specific for STAR:
#define OT_STATIC_R_CDB                 345

// continuation of level 1 (not to resequence ids)
#define OT_STATIC_DBGRANTEES            351
#define OT_DBGRANTEES_SUBSTATICS        352
#define OT_STATIC_DBGRANTEES_ACCESY     353
#define OT_STATIC_DBGRANTEES_ACCESN     354
#define OT_STATIC_DBGRANTEES_CREPRY     355
#define OT_STATIC_DBGRANTEES_CREPRN     356
#define OT_STATIC_DBGRANTEES_CRETBY     357
#define OT_STATIC_DBGRANTEES_CRETBN     358
#define OT_STATIC_DBGRANTEES_DBADMY     359
#define OT_STATIC_DBGRANTEES_DBADMN     360
#define OT_STATIC_DBGRANTEES_LKMODY     361
#define OT_STATIC_DBGRANTEES_LKMODN     362
#define OT_STATIC_DBGRANTEES_QRYIOY     363
#define OT_STATIC_DBGRANTEES_QRYION     364
#define OT_STATIC_DBGRANTEES_QRYRWY     365
#define OT_STATIC_DBGRANTEES_QRYRWN     366
#define OT_STATIC_DBGRANTEES_UPDSCY     367
#define OT_STATIC_DBGRANTEES_UPDSCN     368

#define OT_STATIC_DBGRANTEES_SELSCY     369
#define OT_STATIC_DBGRANTEES_SELSCN     370
#define OT_STATIC_DBGRANTEES_CNCTLY     371
#define OT_STATIC_DBGRANTEES_CNCTLN     372
#define OT_STATIC_DBGRANTEES_IDLTLY     373
#define OT_STATIC_DBGRANTEES_IDLTLN     374
#define OT_STATIC_DBGRANTEES_SESPRY     375
#define OT_STATIC_DBGRANTEES_SESPRN     376
#define OT_STATIC_DBGRANTEES_TBLSTY     377
#define OT_STATIC_DBGRANTEES_TBLSTN     378

#define OT_STATIC_DBALARM                 379
#define OT_STATIC_DBALARM_CONN_SUCCESS    380
#define OT_STATIC_DBALARM_CONN_FAILURE    381
#define OT_STATIC_DBALARM_DISCONN_SUCCESS 382
#define OT_STATIC_DBALARM_DISCONN_FAILURE 383

// new section for previously undocumented dbgrants
#define OT_STATIC_DBGRANTEES_QRYCPY     387
#define OT_STATIC_DBGRANTEES_QRYCPN     388
#define OT_STATIC_DBGRANTEES_QRYPGY     389
#define OT_STATIC_DBGRANTEES_QRYPGN     390
#define OT_STATIC_DBGRANTEES_QRYCOY     391
#define OT_STATIC_DBGRANTEES_QRYCON     392

// level 2, child of "table of database"
#define OT_STATIC_INTEGRITY       400
#define OT_STATIC_RULE            401
#define OT_STATIC_INDEX           402
#define OT_INDEX_SUBSTATICS       403
#define OT_STATIC_TABLELOCATION   404

#define OT_STATIC_SECURITY        405
#define OT_STATIC_SEC_SEL_SUCC    407
#define OT_STATIC_SEC_SEL_FAIL    408
#define OT_STATIC_SEC_DEL_SUCC    410
#define OT_STATIC_SEC_DEL_FAIL    411
#define OT_STATIC_SEC_INS_SUCC    413
#define OT_STATIC_SEC_INS_FAIL    414
#define OT_STATIC_SEC_UPD_SUCC    416
#define OT_STATIC_SEC_UPD_FAIL    417

#define OT_STATIC_R_TABLESYNONYM  418
#define OT_STATIC_R_TABLEVIEW     419

#define OT_STATIC_TABLEGRANTEES       420
#define OT_STATIC_TABLEGRANT_SEL_USER 421
#define OT_STATIC_TABLEGRANT_INS_USER 422
#define OT_STATIC_TABLEGRANT_UPD_USER 423
#define OT_STATIC_TABLEGRANT_DEL_USER 424
#define OT_STATIC_TABLEGRANT_REF_USER 425

#define OT_STATIC_TABLEGRANT_CPI_USER 426
#define OT_STATIC_TABLEGRANT_CPF_USER 427

#define OT_STATIC_TABLEGRANT_ALL_USER 428

// desktop
#define OT_STATIC_TABLEGRANT_INDEX_USER 440
#define OT_STATIC_TABLEGRANT_ALTER_USER 441

// level 2, child of "view of database"
#define OT_STATIC_VIEWTABLE           429
#define OT_STATIC_R_VIEWSYNONYM       430

#define OT_STATIC_VIEWGRANTEES        431
#define OT_STATIC_VIEWGRANT_SEL_USER  432
#define OT_STATIC_VIEWGRANT_INS_USER  433
#define OT_STATIC_VIEWGRANT_UPD_USER  434
#define OT_STATIC_VIEWGRANT_DEL_USER  435

// level 2, child of "procedure of database"
#define OT_STATIC_PROCGRANT_EXEC_USER 436
#define OT_STATIC_R_PROC_RULE         437

// level 2, child of "dbevent of database"
#define OT_STATIC_DBEGRANT_RAISE_USER 438
#define OT_STATIC_DBEGRANT_REGTR_USER 439

// level 3, child of "index on table of database"
#define OT_STATIC_R_INDEXSYNONYM  501

// level 3, child of "rule on table of database"
#define OT_STATIC_RULEPROC        502


// Replicator - start from 551
#define OT_STATIC_REPLICATOR             551
#define OT_STATIC_REPLIC_CONNECTION      552
#define OT_STATIC_REPLIC_CDDS            553
#define OT_STATIC_REPLIC_CDDS_DETAIL     554
#define OT_STATIC_R_REPLIC_CDDS_TABLE    555
#define OT_STATIC_REPLIC_MAILUSER        556
#define OT_STATIC_REPLIC_REGTABLE        557
#define OT_STATIC_R_REPLIC_TABLE_CDDS    558

// new style alarms (with 2 sub-branches alarmee and launched dbevent)
#define OT_STATIC_ALARMEE                560
#define OT_STATIC_ALARM_EVENT            561
#define OT_ALARMEE_SOLVED_USER           562
#define OT_ALARMEE_SOLVED_GROUP          563
#define OT_ALARMEE_SOLVED_ROLE           564

// special items
#define OT_STATIC_DUMMY       601
#define OT_STATIC_ERR_NOGRANT 602
#define OT_STATIC_ERR_TIMEOUT 603
#define OT_STATIC_ERROR       604
#define OT_VIEWTABLE_SOLVE    605
#define OT_GRANTEE_SOLVE      606
#define OT_SYNONYMED_SOLVE    607
#define OT_ALARMEE_SOLVE      608

//
//  STAR TYPES FOR ICONS CUSTOMIZATION ACCORDING TO OBJECTS SUBTYPES
//
// databases:
#define OT_STAR_DB_DDB            609
#define OT_STAR_DB_CDB            610

// tables:
#define OT_STAR_TBL_NATIVE        611
#define OT_STAR_TBL_LINK          612

// views:
#define OT_STAR_VIEW_NATIVE       613
#define OT_STAR_VIEW_LINK         614

// procedures
#define OT_STAR_PROCEDURE         615

// index
#define OT_STAR_INDEX             616

// Note: 617 to 619 used in dba.h
// 3 types for schema subbranches: tables, views and procedures
#define OT_SCHEMAUSER_SUBSTATICS         620
#define OT_STATIC_SCHEMAUSER_TABLE       621
#define OT_STATIC_SCHEMAUSER_VIEW        622
#define OT_STATIC_SCHEMAUSER_PROCEDURE   623

//
// ICE
//
#define OT_STATIC_ICE                      631
// Under "Security"
#define OT_STATIC_ICE_SECURITY             632
#define OT_STATIC_ICE_DBUSER               633
#define OT_STATIC_ICE_DBCONNECTION         634
#define OT_STATIC_ICE_WEBUSER              635
#define OT_STATIC_ICE_WEBUSER_ROLE         636
#define OT_STATIC_ICE_WEBUSER_CONNECTION   637
#define OT_STATIC_ICE_PROFILE              638
#define OT_STATIC_ICE_PROFILE_ROLE         639
#define OT_STATIC_ICE_PROFILE_CONNECTION   640
// Under "Bussiness unit" (BUNIT)
#define OT_STATIC_ICE_BUNIT                641
#define OT_STATIC_ICE_BUNIT_SECURITY       642
#define OT_STATIC_ICE_BUNIT_SEC_ROLE       643
#define OT_STATIC_ICE_BUNIT_SEC_USER       644
#define OT_STATIC_ICE_BUNIT_FACET          645
#define OT_STATIC_ICE_BUNIT_PAGE           646
#define OT_STATIC_ICE_BUNIT_LOCATION       647
// Under "Server"
#define OT_STATIC_ICE_SERVER               648
#define OT_STATIC_ICE_SERVER_APPLICATION   649
#define OT_STATIC_ICE_SERVER_LOCATION      650
#define OT_STATIC_ICE_SERVER_VARIABLE      651
// Substatic defines
#define OT_ICE_SUBSTATICS                  652
#define OT_ICE_SECURITY_SUBSTATICS         653
#define OT_ICE_BUNIT_SUBSTATICS            654
#define OT_ICE_SERVER_SUBSTATICS           655
// Added later
#define OT_STATIC_ICE_ROLE                 656

#define OT_STATIC_ICE_BUNIT_FACET_ROLE     657
#define OT_STATIC_ICE_BUNIT_FACET_USER     658
#define OT_STATIC_ICE_BUNIT_PAGE_ROLE      659
#define OT_STATIC_ICE_BUNIT_PAGE_USER      660

//
// INSTALLATION LEVEL SETTINGS
//
#define OT_STATIC_INSTALL                  661
#define OT_INSTALL_SUBSTATICS              662
#define OT_STATIC_INSTALL_SECURITY         663
// reserved, no sub branch at this time : #define OT_INSTALL_SECURITY_SUBSTATICS     664
#define OT_STATIC_INSTALL_GRANTEES         665
#define OT_INSTALL_GRANTEES_SUBSTATICS     666
#define OT_STATIC_INSTALL_ALARMS           667
#define OT_INSTALL_ALARMS_SUBSTATICS       668

//
// SEQUENCE
//
#define OT_STATIC_SEQUENCE                 669
#define OT_STATIC_SEQGRANT_NEXT_USER       670

#define OT_STATIC_DBGRANTEES_CRSEQY        671
#define OT_STATIC_DBGRANTEES_CRSEQN        672

#define OT_STATIC_R_SEQGRANT               673
#define OT_STATIC_R_SEQGRANT_NEXT          674

#define OT_STATIC_R_DBGRANT_CRESEQY        675
#define OT_STATIC_R_DBGRANT_CRESEQN        676

//
// OT_TABLE for Ingres VectorWise icons
//
#define OT_VW_TABLE	677

/********* WARNING: values after 1000 are used in dba.h ***************/


// functions
extern VOID   InitializeCurrentRecId(LPDOMDATA lpDomData);
extern DWORD  TreeAddRecord(LPDOMDATA lpDomData, LPSTR lpString, DWORD parentId, DWORD insertAfter, DWORD insertBefore, LPVOID lpData);
extern DWORD  AddDummySubItem(LPDOMDATA lpDomData, DWORD idParent);
extern DWORD  AddProblemSubItem(LPDOMDATA lpDomData, DWORD idParent, int errortype, int iobjecttype, LPTREERECORD lpRefRecord);
extern BOOL   TreeDeleteRecord(LPDOMDATA lpDomData, DWORD recordId);
extern VOID   TreeDeleteAllRecords(LPDOMDATA lpDomData);
extern LPTREERECORD AllocAndFillRecord(int recType, BOOL bSubValid, LPSTR psz1, LPSTR psz2, LPSTR psz3, int parentDbType, LPSTR pszObjName, LPSTR pszOwner, LPSTR pszComplim, LPSTR pszTableOwner);
extern UINT   GetObjectTypeStringId(int recType);
extern UINT   GetStaticStringId(int staticType);
extern UINT   GetStaticStatusStringId(int staticType);
extern BOOL   DomTreeNotifMsg(HWND hwndMdi, UINT wMsg, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam, LONG *plRet);
extern DWORD  GetAnchorId(LPDOMDATA lpDomData, int iobjecttype, int level, LPUCHAR *parentstrings, DWORD previousId, LPUCHAR bufPar1, LPUCHAR bufPar2, LPUCHAR bufPar3, int iAction, DWORD idItem, BOOL bTryCurrent, BOOL *pbRootItem);
extern UCHAR *GetObjName(LPDOMDATA lpDomData, DWORD dwItem);
extern DWORD  GetCurSel(LPDOMDATA lpDomData);
extern int    GetCurSelObjType(LPDOMDATA lpDomData);
extern int    GetItemObjType(LPDOMDATA lpDomData, DWORD dwItem);
extern int    GetUnsolvedItemObjType(LPDOMDATA lpDomData, DWORD dwItem);
extern int    GetItemDisplayObjType(LPDOMDATA lpDomData, DWORD dwItem);
extern BOOL   IsCurSelObjStatic(LPDOMDATA lpDomData);
extern BOOL   IsItemObjStatic(LPDOMDATA lpDomData, DWORD dwItem);
extern DWORD  GetFirstImmediateChild(LPDOMDATA lpDomData, DWORD idParentReq, DWORD *pParentLevel);
extern DWORD  GetNextImmediateChild(LPDOMDATA lpDomData, DWORD idParentReq, DWORD idCurChild, DWORD dwParentLevel);
extern DWORD  FindImmediateSubItem(LPDOMDATA lpDomData, DWORD idParentReq, DWORD idFirstChildObj, DWORD idLastChildObj, int iobjecttype, LPUCHAR objName,
                                   LPUCHAR objOwnerName, LPUCHAR parentName, LPUCHAR bufComplim, DWORD *pidInsertAfter, DWORD *pidInsertBefore);
extern VOID   TreeDeleteAllChildren(LPDOMDATA lpDomData, DWORD idParent);
extern VOID   TreeDeleteAllMarkedChildren(LPDOMDATA lpDomData, DWORD idParent, DWORD idLastChildObj);
extern long   GetNumberOfObjects(LPDOMDATA lpDomData, BOOL bForce);
extern DWORD  GetPreviousSibling(LPDOMDATA lpDomData, DWORD dwReq);
extern DWORD  GetNextSibling(LPDOMDATA lpDomData, DWORD dwReq);
extern DWORD  GetLastRecordId(LPDOMDATA lpDomData);
extern DWORD  GetFirstLevel0Item(LPDOMDATA lpDomData);
extern DWORD  GetNextLevel0Item(LPDOMDATA lpDomData, DWORD idCurItem);

//  On-the-fly type solve, ONLY for TearOut/RestartFromPos and Clipboard
extern int  SolveType(LPDOMDATA lpDomData, LPTREERECORD lpRecord, BOOL bUpdateRec);

void UpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bUpdate, int initialSel);

// properties pane - different from HasProperties4Display() since new types
BOOL HasPropertiesPane(int objType);

// for star: encapsulates IsSystemObject()
extern BOOL IsSystemObject2(int iobjecttype,LPUCHAR lpobjectname,LPUCHAR lpobjectowner, int parentDbType);

// for dragdrop from right pane
extern BOOL RPaneDragDropMousemove(HWND hwndMdi, int x, int y);
extern BOOL RPaneDragDropEnd(HWND hwndMdi, int x, int y, LPTREERECORD lpRightPaneRecord);
extern BOOL ManageDragDropStartFromRPane(HWND hwndMdi, int rPaneObjType, LPUCHAR objName, LPUCHAR ownerName);

// for multithreaded drag-drop
int StandardDragDropEnd_MT(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDesthwndDomDest, LPDOMDATA lpDestDomData);

// For multithreaded Update of right pane
void MT_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear);
void TS_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear);

#endif //__TREE_INCLUDED__
