#ifdef WIN32
#define WIN95_CONTROLS
#endif
/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : tree2.c - splitted May 3, 95 from tree.c
//    manages the lbtree inside a dom type mdi document
//
//    Author : Emmanuel Blattes
//
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  18-Mar-2003 (schph01)
**   sir 107523 Add Branches 'Sequence' and 'Grantees Sequence'
**  10-Nov-2004 (schph01)
**   (bug 113426) manage in GetAnchorId() function the type OT_ICE_MAIN
**   and OT_ICE_SUBSTATICS type.
**  11-Apr-2005 (lazro01)
**   (bug 114273) Optimized code changes for finding the id of the 
**   the immediate child of a branch during Forced Refresh.
********************************************************************/

//
// Includes
//
// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "domdisp.h"
#include "dbaginfo.h"
#include "main.h"
#include "dom.h"
#include "resource.h"
#include "treelb.e"   // tree listbox dll
// Specific for Dom with splitbar
extern void FreeDomPageInfo(HWND hwndDom, DWORD m_pPageInfo);   // in mainvi2.cpp

//
// global variables
//

//
// static functions definition
//
static VOID  NEAR UpdateCurrentRecId(LPDOMDATA lpDomData);
static DWORD NEAR GetCurrentRecId(LPDOMDATA lpDomData);

//
//  Lbtree record id management functions
//
VOID InitializeCurrentRecId(LPDOMDATA lpDomData)
{
  lpDomData->currentRecId = 1UL;
}

static DWORD NEAR GetCurrentRecId(LPDOMDATA lpDomData)
{
  return lpDomData->currentRecId;
}

// assumption on 32 bits!!! - check against machine
#define TOP_SEUIL (0xFFFFFFFF - 1000000000)
// for debug - static DWORD TOP_SEUIL = 100;

static VOID NEAR UpdateCurrentRecId(LPDOMDATA lpDomData)
{
  // We definitely can assume we will never reach the upper limit
  lpDomData->currentRecId++;

  // Anyhow, we flag the user if we are near the upper limit every 1000 recid
  // plus set bMustResequence to automatically reseq. the tree
  if (lpDomData->currentRecId > TOP_SEUIL ) {
    if (!(lpDomData->bMustResequence)) {
      char sz[BUFSIZE];
      HWND hwndFocus;
      LoadString (hResource, IDS_DOMDOC_LOWONRESOURCE, sz, sizeof(sz));
      hwndFocus = GetFocus();
      MessageBox( hwndFocus, sz, NULL,
                  MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      SetFocus(hwndFocus);
    }
    lpDomData->bMustResequence = TRUE;
  }
}

//
//  Record creation
//
//  lpData MUST have been allocated through ESL_AllocMem since it is freed
//  in TreeDeleteRecord, OR MUST be null
//
//  16/3/95 : Important modification for sorted insertion:
//  We will insert before if the "insertBefore" treeLineId is not null,
//  otherwise we will insert after using the "insertAfter" treeLineId
//
//  lpString is the caption, which may be different from the object name
//
DWORD TreeAddRecord(LPDOMDATA lpDomData, LPSTR lpString, DWORD parentId, DWORD insertAfter, DWORD insertBefore, LPVOID lpData)
{
  LINK_RECORD lr;
  BOOL        bRet;
  DWORD       recId;

  lr.lpString       = lpString;
  lr.ulRecID        = recId = GetCurrentRecId(lpDomData);
  lr.ulParentID     = parentId;
  lr.lpItemData     = lpData;

  if (insertBefore) {
    lr.ulInsertAfter  = insertBefore;
    bRet = (BOOL) SendMessage(lpDomData->hwndTreeLb, LM_ADDRECORDBEFORE,
                              0, (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr);
  }
  else {
    lr.ulInsertAfter  = insertAfter;
    bRet = (BOOL) SendMessage(lpDomData->hwndTreeLb, LM_ADDRECORD,
                              0, (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr);
  }
  if (!bRet) {
    // not succesful : free item data and return
    if (lpData)
      ESL_FreeMem (lpData);
    return 0;
  }
  UpdateCurrentRecId(lpDomData);
  return recId;
}

//
//  Record destruction
//
BOOL TreeDeleteRecord(LPDOMDATA lpDomData, DWORD recordId)
{
  LPTREERECORD lpItemData;

  lpItemData = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA,
                                    0, (LPARAM)recordId);
  if (lpItemData) {
    // Added Nov 26 for right pane (dom with splitbar)
    if (lpItemData->bPageInfoCreated) {
      if (lpItemData->m_pPageInfo)
        FreeDomPageInfo(GetParent(lpDomData->hwndTreeLb), lpItemData->m_pPageInfo);
    }
    ESL_FreeMem (lpItemData);
  }
  SendMessage(lpDomData->hwndTreeLb, LM_DELETERECORD, 0, (LPARAM)recordId);

  // Always return TRUE since LM_DELETERECORD always returns this value
  return TRUE;
}

//
//  All remaining records destruction
//
VOID TreeDeleteAllRecords(LPDOMDATA lpDomData)
{
  DWORD curRecordId;
  LPTREERECORD lpItemData;

  curRecordId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
  while (curRecordId) {
    lpItemData = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA,
                                      0, (LPARAM)curRecordId);
    if (lpItemData) {
      // Added Nov 26 for right pane (dom with splitbar)
      if (lpItemData->bPageInfoCreated) {
        if (lpItemData->m_pPageInfo)
          FreeDomPageInfo(GetParent(lpDomData->hwndTreeLb), lpItemData->m_pPageInfo);
      }
      ESL_FreeMem (lpItemData);
    }
    curRecordId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, 0L);
  }

  SendMessage(lpDomData->hwndTreeLb, LM_DELETEALL, 0, 0L);
}

//
// LPTREERECORD AllocAndFillRecord(int recType, BOOL bSubValid, LPSTR psz1, LPSTR psz2, LPSTR psz3, int parentDbType, LPSTR pszObjName, LPSTR pszOwner, LPSTR pszComplim, LPSTR pszTableOwner)
//
// Allocates a Tree Record, and fills it with received data
//
// psz1, psz2 and psz3 are the parenthood strings
// parentDbType is the star type of the parent database, if any, otherwise 0
// pszObjName is the name of the object returned by DomGetFirst/Next
// pszOwner is owner data returned by DomGetFirst/Next
// pszComplim is complimentary data returned by DomGetFirst/Next
//
LPTREERECORD AllocAndFillRecord(int recType, BOOL bSubValid, LPSTR psz1, LPSTR psz2, LPSTR psz3, int parentDbType, LPSTR pszObjName, LPSTR pszOwner, LPSTR pszComplim, LPSTR pszTableOwner)
{
  LPTREERECORD lpRecord;

  lpRecord = (LPTREERECORD) ESL_AllocMem(sizeof(TREERECORD));
  if (!lpRecord)
    return lpRecord;

  lpRecord->recType       = recType;
  lpRecord->bSubValid     = bSubValid;
  lpRecord->bToBeDeleted  = FALSE;
  lpRecord->complimValue  = 0L;
  lpRecord->wildcardFilter[0]= '\0';// not mandatory as buffer filled with 0's
  // parenthood
  if (psz1)
    fstrncpy(lpRecord->extra, psz1, MAXOBJECTNAME);
  else
    lpRecord->extra[0] = '\0';
  if (psz2)
    fstrncpy(lpRecord->extra2, psz2, MAXOBJECTNAME);
  else
    lpRecord->extra2[0] = '\0';
  if (psz3)
    fstrncpy(lpRecord->extra3, psz3, MAXOBJECTNAME);
  else
    lpRecord->extra3[0] = '\0';
  // Star parent db type
  lpRecord->parentDbType = parentDbType;

  // object name - caution in case we received a NULL pointer
  if (pszObjName)
    fstrncpy(lpRecord->objName, pszObjName, MAXOBJECTNAME);
  else
    lstrcpy(lpRecord->objName, "<ERROR - NO OBJECT NAME>");

  // object owner
  if (pszOwner)
    fstrncpy(lpRecord->ownerName, pszOwner, MAXOBJECTNAME);
  else
    lpRecord->ownerName[0] = '\0';

  // object complimentary data
  // Fix Oct 15, 97: complimentary data can contain extradatas ---> use memcpy instead of string copy
  if (pszComplim)
    memcpy(lpRecord->szComplim, pszComplim, sizeof(lpRecord->szComplim));
  else
    lpRecord->szComplim[0] = '\0';

  // if parent of object is a table, schema for the table
  if (pszTableOwner)
    fstrncpy(lpRecord->tableOwner, pszTableOwner, MAXOBJECTNAME);
  else
    lpRecord->tableOwner[0] = '\0';

  lpRecord->unsolvedRecType = -1;

  return lpRecord;
}

//
// returns the id of a string that will be displayed in the status bar
//
UINT GetObjectTypeStringId(int recType)
{
  switch (recType) {
    // level 0
    case OT_DATABASE:
      return IDS_TREE_STATUS_OT_DATABASE;
    case OT_PROFILE:
      return IDS_TREE_STATUS_OT_PROFILE;
    case OT_USER:
      return IDS_TREE_STATUS_OT_USER;
    case OT_GRANTEE_SOLVED_USER:
      return IDS_TREE_STATUS_OT_GRANTEE_SOLVED_USER;
    case OT_ALARMEE_SOLVED_USER:
      return IDS_TREE_STATUS_OT_ALARMEE_SOLVED_USER;
    case OT_GROUP:
      return IDS_TREE_STATUS_OT_GROUP;
    case OT_GRANTEE_SOLVED_GROUP:
      return IDS_TREE_STATUS_OT_GRANTEE_SOLVED_GROUP;
    case OT_ALARMEE_SOLVED_GROUP:
      return IDS_TREE_STATUS_OT_ALARMEE_SOLVED_GROUP;
    case OT_ROLE:
      return IDS_TREE_STATUS_OT_ROLE;
    case OT_GRANTEE_SOLVED_ROLE:
      return IDS_TREE_STATUS_OT_GRANTEE_SOLVED_ROLE;
    case OT_ALARMEE_SOLVED_ROLE:
      return IDS_TREE_STATUS_OT_ALARMEE_SOLVED_ROLE;
    case OT_LOCATION:
      return IDS_TREE_STATUS_OT_LOCATION;
    // pseudo level 0
    case OT_SYNONYMOBJECT:
      return IDS_TREE_STATUS_OT_SYNONYMED;

    // level 1, child of database
    case OT_TABLE:
      return IDS_TREE_STATUS_OT_TABLE;
    case OT_VIEW:
      return IDS_TREE_STATUS_OT_VIEW;
    case OT_PROCEDURE:
      return IDS_TREE_STATUS_OT_PROCEDURE;
    case OT_SEQUENCE:
      return IDS_TREE_STATUS_OT_SEQUENCE;
    case OT_SCHEMAUSER:
      return IDS_TREE_STATUS_OT_SCHEMAUSER;
    case OT_SYNONYM:
      return IDS_TREE_STATUS_OT_SYNONYM;
    case OT_DBGRANT_ACCESY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_ACCESY_USER;
    case OT_DBGRANT_ACCESN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_ACCESN_USER;
    case OT_DBGRANT_CREPRY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CREPRY_USER;
    case OT_DBGRANT_CREPRN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CREPRN_USER;
    case OT_DBGRANT_CRETBY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CRETBY_USER;
    case OT_DBGRANT_CRETBN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CRETBN_USER;
    case OT_DBGRANT_DBADMY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_DBADMY_USER;
    case OT_DBGRANT_DBADMN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_DBADMN_USER;
    case OT_DBGRANT_LKMODY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_LKMODY_USER;
    case OT_DBGRANT_LKMODN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_LKMODN_USER;
    case OT_DBGRANT_QRYIOY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYIOY_USER;
    case OT_DBGRANT_QRYION_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYION_USER;
    case OT_DBGRANT_QRYRWY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYRWY_USER;
    case OT_DBGRANT_QRYRWN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYRWN_USER;
    case OT_DBGRANT_UPDSCY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_UPDSCY_USER;
    case OT_DBGRANT_UPDSCN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_UPDSCN_USER;

    case OT_DBGRANT_SELSCY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_SELSCY_USER;
    case OT_DBGRANT_SELSCN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_SELSCN_USER;
    case OT_DBGRANT_CNCTLY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CNCTLY_USER;
    case OT_DBGRANT_CNCTLN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_CNCTLN_USER;
    case OT_DBGRANT_IDLTLY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_IDLTLY_USER;
    case OT_DBGRANT_IDLTLN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_IDLTLN_USER;
    case OT_DBGRANT_SESPRY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_SESPRY_USER;
    case OT_DBGRANT_SESPRN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_SESPRN_USER;
    case OT_DBGRANT_TBLSTY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_TBLSTY_USER;
    case OT_DBGRANT_TBLSTN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_TBLSTN_USER;

    case OT_DBGRANT_QRYCPY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCPY_USER;
    case OT_DBGRANT_QRYCPN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCPN_USER;
    case OT_DBGRANT_QRYPGY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYPGY_USER;
    case OT_DBGRANT_QRYPGN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYPGN_USER;
    case OT_DBGRANT_QRYCOY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCOY_USER;
    case OT_DBGRANT_QRYCON_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCON_USER;
    case OT_DBGRANT_SEQCRY_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCOY_USER;
    case OT_DBGRANT_SEQCRN_USER:
      return IDS_TREE_STATUS_OT_DBGRANT_QRYCON_USER;

    case OT_DBEVENT:
      return IDS_TREE_STATUS_OT_DBEVENT;

    case OT_S_ALARM_CO_SUCCESS_USER:
      return IDS_TREE_STATUS_OT_DBALARM_CONN_SUCCESS;
    case OT_S_ALARM_CO_FAILURE_USER:
      return IDS_TREE_STATUS_OT_DBALARM_CONN_FAILURE;
    case OT_S_ALARM_DI_SUCCESS_USER:
      return IDS_TREE_STATUS_OT_DBALARM_DISCONN_SUCCESS;
    case OT_S_ALARM_DI_FAILURE_USER:
      return IDS_TREE_STATUS_OT_DBALARM_DISCONN_FAILURE;

    case OTR_CDB:
      return IDS_TREE_STATUS_OTR_CDB;

    // level 1, child of user
    case OTR_USERSCHEMA:
      return IDS_TREE_STATUS_OTR_USERSCHEMA;
    case OTR_USERGROUP:
      return IDS_TREE_STATUS_OTR_USERGROUP;
    case OTR_ALARM_SELSUCCESS_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_SELSUCCESS_TABLE;
    case OTR_ALARM_SELFAILURE_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_SELFAILURE_TABLE;
    case OTR_ALARM_DELSUCCESS_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_DELSUCCESS_TABLE;
    case OTR_ALARM_DELFAILURE_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_DELFAILURE_TABLE;
    case OTR_ALARM_INSSUCCESS_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_INSSUCCESS_TABLE;
    case OTR_ALARM_INSFAILURE_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_INSFAILURE_TABLE;
    case OTR_ALARM_UPDSUCCESS_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_UPDSUCCESS_TABLE;
    case OTR_ALARM_UPDFAILURE_TABLE:
      return IDS_TREE_STATUS_OTR_ALARM_UPDFAILURE_TABLE;

    case OTR_GRANTEE_RAISE_DBEVENT:
      return IDS_TREE_STATUS_OTR_GRANTEE_RAISE_DBEVENT;
    case OTR_GRANTEE_REGTR_DBEVENT:
      return IDS_TREE_STATUS_OTR_GRANTEE_REGTR_DBEVENT;
    case OTR_GRANTEE_EXEC_PROC:
      return IDS_TREE_STATUS_OTR_GRANTEE_EXEC_PROC;
    case OTR_GRANTEE_NEXT_SEQU:
      return IDS_TREE_STATUS_OTR_GRANTEE_NEXT_SEQU;
    case OTR_DBGRANT_ACCESY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_ACCESY_DB;
    case OTR_DBGRANT_ACCESN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_ACCESN_DB;
    case OTR_DBGRANT_CREPRY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CREPRY_DB;
    case OTR_DBGRANT_CREPRN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CREPRN_DB;
    case OTR_DBGRANT_SEQCRY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SEQCRY_DB;
    case OTR_DBGRANT_SEQCRN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SEQCRN_DB;
    case OTR_DBGRANT_CRETBY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CRETBY_DB;
    case OTR_DBGRANT_CRETBN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CRETBN_DB;
    case OTR_DBGRANT_DBADMY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_DBADMY_DB;
    case OTR_DBGRANT_DBADMN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_DBADMN_DB;
    case OTR_DBGRANT_LKMODY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_LKMODY_DB;
    case OTR_DBGRANT_LKMODN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_LKMODN_DB;
    case OTR_DBGRANT_QRYIOY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYIOY_DB;
    case OTR_DBGRANT_QRYION_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYION_DB;
    case OTR_DBGRANT_QRYRWY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYRWY_DB;
    case OTR_DBGRANT_QRYRWN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYRWN_DB;
    case OTR_DBGRANT_UPDSCY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_UPDSCY_DB;
    case OTR_DBGRANT_UPDSCN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_UPDSCN_DB;

    case OTR_DBGRANT_SELSCY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SELSCY_DB;
    case OTR_DBGRANT_SELSCN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SELSCN_DB;
    case OTR_DBGRANT_CNCTLY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CNCTLY_DB;
    case OTR_DBGRANT_CNCTLN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_CNCTLN_DB;
    case OTR_DBGRANT_IDLTLY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_IDLTLY_DB;
    case OTR_DBGRANT_IDLTLN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_IDLTLN_DB;
    case OTR_DBGRANT_SESPRY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SESPRY_DB;
    case OTR_DBGRANT_SESPRN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_SESPRN_DB;
    case OTR_DBGRANT_TBLSTY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_TBLSTY_DB;
    case OTR_DBGRANT_TBLSTN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_TBLSTN_DB;

    case OTR_DBGRANT_QRYCPY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYCPY_DB;
    case OTR_DBGRANT_QRYCPN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYCPN_DB;
    case OTR_DBGRANT_QRYPGY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYPGY_DB;
    case OTR_DBGRANT_QRYPGN_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYPGN_DB;
    case OTR_DBGRANT_QRYCOY_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYCOY_DB;
    case OTR_DBGRANT_QRYCON_DB:
      return IDS_TREE_STATUS_OTR_DBGRANT_QRYCON_DB;

    case OTR_GRANTEE_SEL_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_SEL_TABLE;
    case OTR_GRANTEE_INS_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_INS_TABLE;
    case OTR_GRANTEE_UPD_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_UPD_TABLE;
    case OTR_GRANTEE_DEL_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_DEL_TABLE;
    case OTR_GRANTEE_REF_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_REF_TABLE;
    case OTR_GRANTEE_CPI_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_CPI_TABLE;
    case OTR_GRANTEE_CPF_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_CPF_TABLE;
    case OTR_GRANTEE_ALL_TABLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_ALL_TABLE;
    case OTR_GRANTEE_SEL_VIEW:
      return IDS_TREE_STATUS_OTR_GRANTEE_SEL_VIEW;
    case OTR_GRANTEE_INS_VIEW:
      return IDS_TREE_STATUS_OTR_GRANTEE_INS_VIEW;
    case OTR_GRANTEE_UPD_VIEW:
      return IDS_TREE_STATUS_OTR_GRANTEE_UPD_VIEW;
    case OTR_GRANTEE_DEL_VIEW:
      return IDS_TREE_STATUS_OTR_GRANTEE_DEL_VIEW;
    case OTR_GRANTEE_ROLE:
      return IDS_TREE_STATUS_OTR_GRANTEE_ROLE;

    // level 1, child of group
    case OT_GROUPUSER:
      return IDS_TREE_STATUS_OT_GROUPUSER;

    // level 1, child of role
    case OT_ROLEGRANT_USER:
      return IDS_TREE_STATUS_OT_ROLEGRANT_USER;

    // level 1, child of location
    case OTR_LOCATIONTABLE:
      return IDS_TREE_STATUS_OTR_LOCATIONTABLE;

    // level 2, child of "table of database"
    case OT_INTEGRITY:
      return IDS_TREE_STATUS_OT_INTEGRITY;
    case OT_RULE:
      return IDS_TREE_STATUS_OT_RULE;
    case OT_INDEX:
      return IDS_TREE_STATUS_OT_INDEX;
    case OT_TABLELOCATION:
      return IDS_TREE_STATUS_OT_TABLELOCATION;

    case OT_S_ALARM_SELSUCCESS_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_SELSUCCESS_USER;
    case OT_S_ALARM_SELFAILURE_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_SELFAILURE_USER;
    case OT_S_ALARM_DELSUCCESS_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_DELSUCCESS_USER;
    case OT_S_ALARM_DELFAILURE_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_DELFAILURE_USER;
    case OT_S_ALARM_INSSUCCESS_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_INSSUCCESS_USER;
    case OT_S_ALARM_INSFAILURE_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_INSFAILURE_USER;
    case OT_S_ALARM_UPDSUCCESS_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_UPDSUCCESS_USER;
    case OT_S_ALARM_UPDFAILURE_USER:
      return IDS_TREE_STATUS_OT_S_ALARM_UPDFAILURE_USER;

    case OT_TABLEGRANT_SEL_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_SEL_USER;
    case OT_TABLEGRANT_INS_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_INS_USER;
    case OT_TABLEGRANT_UPD_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_UPD_USER;
    case OT_TABLEGRANT_DEL_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_DEL_USER;
    case OT_TABLEGRANT_REF_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_REF_USER;
    case OT_TABLEGRANT_CPI_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_CPI_USER;
    case OT_TABLEGRANT_CPF_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_CPF_USER;
    case OT_TABLEGRANT_ALL_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_ALL_USER;
    // desktop
    case OT_TABLEGRANT_INDEX_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_INDEX_USER;
    // desktop
    case OT_TABLEGRANT_ALTER_USER:
      return IDS_TREE_STATUS_OT_TABLEGRANT_ALTER_USER;

    case OTR_TABLESYNONYM:
      return IDS_TREE_STATUS_OTR_TABLESYNONYM;
    case OTR_TABLEVIEW:
      return IDS_TREE_STATUS_OTR_TABLEVIEW;

    // level 2, child of "view of database"
    case OT_VIEWTABLE:
      return IDS_TREE_STATUS_OT_VIEWTABLE;
    case OTR_VIEWSYNONYM:
      return IDS_TREE_STATUS_OTR_VIEWSYNONYM;

    case OT_VIEWGRANT_SEL_USER:
      return IDS_TREE_STATUS_OT_VIEWGRANT_SEL_USER;
    case OT_VIEWGRANT_INS_USER:
      return IDS_TREE_STATUS_OT_VIEWGRANT_INS_USER;
    case OT_VIEWGRANT_UPD_USER:
      return IDS_TREE_STATUS_OT_VIEWGRANT_UPD_USER;
    case OT_VIEWGRANT_DEL_USER:
      return IDS_TREE_STATUS_OT_VIEWGRANT_DEL_USER;

    // level 2, child of "procedure of database"
    case OT_PROCGRANT_EXEC_USER:
      return IDS_TREE_STATUS_OT_PROCGRANT_EXEC_USER;
    case OTR_PROC_RULE:
      return IDS_TREE_STATUS_OTR_PROC_RULE;

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:
      return IDS_TREE_STATUS_OT_GRANTSEQ;

    // level 2, child of "dbevent of database"
    case OT_DBEGRANT_RAISE_USER:
      return IDS_TREE_STATUS_OT_DBEGRANT_RAISE_USER;
    case OT_DBEGRANT_REGTR_USER:
      return IDS_TREE_STATUS_OT_DBEGRANT_REGTR_USER;

    // level 2, child of "schemauser on database"
    case OT_SCHEMAUSER_TABLE:
      return IDS_TREE_STATUS_OT_SCHEMAUSER_TABLE;
    case OT_SCHEMAUSER_VIEW:
      return IDS_TREE_STATUS_OT_SCHEMAUSER_VIEW;
    case OT_SCHEMAUSER_PROCEDURE:
      return IDS_TREE_STATUS_OT_SCHEMAUSER_PROCEDURE;

    // level 3, child of "index on table of database"
    case OTR_INDEXSYNONYM:
      return IDS_TREE_STATUS_OTR_INDEXSYNONYM;

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:
      return IDS_TREE_STATUS_OT_RULEPROC;

    // replicator, all levels mixed
    case OT_REPLICATOR:
      return IDS_TREE_STATUS_OT_REPLICATOR;

    case OT_REPLIC_CONNECTION:
      return IDS_TREE_STATUS_OT_REPLIC_CONNECTION;
    case OT_REPLIC_CDDS:
      return IDS_TREE_STATUS_OT_REPLIC_CDDS;
    case OT_REPLIC_MAILUSER:
      return IDS_TREE_STATUS_OT_REPLIC_MAILUSER;
    case OT_REPLIC_REGTABLE:
      return IDS_TREE_STATUS_OT_REPLIC_REGTABLE;

    case OT_REPLIC_CDDS_DETAIL:
      return IDS_TREE_STATUS_OT_REPLIC_CDDS_DETAIL;
    case OTR_REPLIC_CDDS_TABLE:
      return IDS_TREE_STATUS_OTR_REPLIC_CDDS_TABLE;

    case OTR_REPLIC_TABLE_CDDS:
      return IDS_TREE_STATUS_OTR_REPLIC_TABLE_CDDS;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_ALARMEE:
      return IDS_TREE_STATUS_OT_ALARMEE;
    case OT_S_ALARM_EVENT:
      return IDS_TREE_STATUS_OT_ALARM_EVENT;

    //
    // ICE
    //
    // Under "Security"
    case OT_ICE_ROLE               :
      return IDS_TREE_STATUS_OT_ICE_ROLE;
    case OT_ICE_DBUSER               :
      return IDS_TREE_STATUS_OT_ICE_DBUSER;
    case OT_ICE_DBCONNECTION         :
      return IDS_TREE_STATUS_OT_ICE_DBCONNECTION;
    case OT_ICE_WEBUSER              :
      return IDS_TREE_STATUS_OT_ICE_WEBUSER;
    case OT_ICE_WEBUSER_ROLE         :
      return IDS_TREE_STATUS_OT_ICE_WEBUSER_ROLE;
    case OT_ICE_WEBUSER_CONNECTION   :
      return IDS_TREE_STATUS_OT_ICE_WEBUSER_CONNECTION;
    case OT_ICE_PROFILE              :
      return IDS_TREE_STATUS_OT_ICE_PROFILE;
    case OT_ICE_PROFILE_ROLE         :
      return IDS_TREE_STATUS_OT_ICE_PROFILE_ROLE;
    case OT_ICE_PROFILE_CONNECTION   :
      return IDS_TREE_STATUS_OT_ICE_PROFILE_CONNECTION;
    // Under "Bussiness unit" (BUNIT)
    case OT_ICE_BUNIT                :
      return IDS_TREE_STATUS_OT_ICE_BUNIT;
    case OT_ICE_BUNIT_SEC_ROLE       :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_SEC_ROLE;
    case OT_ICE_BUNIT_SEC_USER       :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_SEC_USER;
    case OT_ICE_BUNIT_FACET          :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_FACET;
    case OT_ICE_BUNIT_PAGE           :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE;
    case OT_ICE_BUNIT_FACET_ROLE     :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_FACET_ROLE;
    case OT_ICE_BUNIT_FACET_USER     :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_FACET_USER;
    case OT_ICE_BUNIT_PAGE_ROLE     :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE_ROLE;
    case OT_ICE_BUNIT_PAGE_USER     :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_PAGE_USER;
    case OT_ICE_BUNIT_LOCATION       :
      return IDS_TREE_STATUS_OT_ICE_BUNIT_LOCATION;
    // Under "Server"
    case OT_ICE_SERVER_APPLICATION   :
      return IDS_TREE_STATUS_OT_ICE_SERVER_APPLICATION;
    case OT_ICE_SERVER_LOCATION      :
      return IDS_TREE_STATUS_OT_ICE_SERVER_LOCATION;
    case OT_ICE_SERVER_VARIABLE      :
      return IDS_TREE_STATUS_OT_ICE_SERVER_VARIABLE;

    default:
      return IDS_TREE_STATUS_OT_UNKNOWN;
  }
}

//
// Returns the "object name" string id for a static type,
// used to display in the tree line
//
UINT GetStaticStringId(int staticType)
{
  UINT  stringId;

  switch (staticType) {
    // level 0
    case OT_STATIC_DATABASE:
      stringId = IDS_TREE_DATABASE_STATIC;
      break;
    case OT_STATIC_PROFILE:
      stringId = IDS_TREE_PROFILE_STATIC;
      break;
    case OT_STATIC_USER:
      stringId = IDS_TREE_USER_STATIC;
      break;
    case OT_STATIC_GROUP:
      stringId = IDS_TREE_GROUP_STATIC;
      break;
    case OT_STATIC_ROLE:
      stringId = IDS_TREE_ROLE_STATIC;
      break;
    case OT_STATIC_LOCATION:
      stringId = IDS_TREE_LOCATION_STATIC;
      break;
    case OT_STATIC_SYNONYMED:
      stringId = IDS_TREE_SYNONYMED_STATIC;
      break;

    // level 1, child of database
    case OT_STATIC_TABLE:
      stringId = IDS_TREE_TABLE_STATIC;
      break;
    case OT_STATIC_VIEW:
      stringId = IDS_TREE_VIEW_STATIC;
      break;
    case OT_STATIC_PROCEDURE:
      stringId = IDS_TREE_PROCEDURE_STATIC;
      break;
    case OT_STATIC_SEQUENCE:
      stringId = IDS_TREE_SEQUENCE_STATIC;
      break;
    case OT_STATIC_SCHEMA:
      stringId = IDS_TREE_SCHEMA_STATIC;
      break;
    case OT_STATIC_SYNONYM:
      stringId = IDS_TREE_SYNONYM_STATIC;
      break;
    case OT_STATIC_DBGRANTEES:
      stringId = IDS_TREE_DBGRANTEES_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_ACCESY:
      stringId = IDS_TREE_DBGRANTEES_ACCESY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_ACCESN:
      stringId = IDS_TREE_DBGRANTEES_ACCESN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CREPRY:
      stringId = IDS_TREE_DBGRANTEES_CREPRY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CREPRN:
      stringId = IDS_TREE_DBGRANTEES_CREPRN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CRETBY:
      stringId = IDS_TREE_DBGRANTEES_CRETBY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CRETBN:
      stringId = IDS_TREE_DBGRANTEES_CRETBN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_DBADMY:
      stringId = IDS_TREE_DBGRANTEES_DBADMY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_DBADMN:
      stringId = IDS_TREE_DBGRANTEES_DBADMN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_LKMODY:
      stringId = IDS_TREE_DBGRANTEES_LKMODY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_LKMODN:
      stringId = IDS_TREE_DBGRANTEES_LKMODN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYIOY:
      stringId = IDS_TREE_DBGRANTEES_QRYIOY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYION:
      stringId = IDS_TREE_DBGRANTEES_QRYION_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYRWY:
      stringId = IDS_TREE_DBGRANTEES_QRYRWY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYRWN:
      stringId = IDS_TREE_DBGRANTEES_QRYRWN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_UPDSCY:
      stringId = IDS_TREE_DBGRANTEES_UPDSCY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_UPDSCN:
      stringId = IDS_TREE_DBGRANTEES_UPDSCN_STATIC;
      break;

    case OT_STATIC_DBGRANTEES_SELSCY:
      stringId = IDS_TREE_DBGRANTEES_SELSCY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_SELSCN:
      stringId = IDS_TREE_DBGRANTEES_SELSCN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CNCTLY:
      stringId = IDS_TREE_DBGRANTEES_CNCTLY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CNCTLN:
      stringId = IDS_TREE_DBGRANTEES_CNCTLN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_IDLTLY:
      stringId = IDS_TREE_DBGRANTEES_IDLTLY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_IDLTLN:
      stringId = IDS_TREE_DBGRANTEES_IDLTLN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_SESPRY:
      stringId = IDS_TREE_DBGRANTEES_SESPRY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_SESPRN:
      stringId = IDS_TREE_DBGRANTEES_SESPRN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_TBLSTY:
      stringId = IDS_TREE_DBGRANTEES_TBLSTY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_TBLSTN:
      stringId = IDS_TREE_DBGRANTEES_TBLSTN_STATIC;
      break;

    case OT_STATIC_DBGRANTEES_QRYCPY:
      stringId = IDS_TREE_DBGRANTEES_QRYCPY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYCPN:
      stringId = IDS_TREE_DBGRANTEES_QRYCPN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYPGY:
      stringId = IDS_TREE_DBGRANTEES_QRYPGY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYPGN:
      stringId = IDS_TREE_DBGRANTEES_QRYPGN_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYCOY:
      stringId = IDS_TREE_DBGRANTEES_QRYCOY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_QRYCON:
      stringId = IDS_TREE_DBGRANTEES_QRYCON_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQY:
      stringId = IDS_TREE_DBGRANTEES_CRSEQY_STATIC;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQN:
      stringId = IDS_TREE_DBGRANTEES_CRSEQN_STATIC;
      break;
    case OT_STATIC_DBEVENT:
      stringId = IDS_TREE_DBEVENT_STATIC;
      break;
    case OT_STATIC_DBALARM:
      stringId = IDS_TREE_DBALARM_STATIC;
      break;
    case OT_STATIC_DBALARM_CONN_SUCCESS:
      stringId = IDS_TREE_DBALARM_CONN_SUCCESS_STATIC;
      break;
    case OT_STATIC_DBALARM_CONN_FAILURE:
      stringId = IDS_TREE_DBALARM_CONN_FAILURE_STATIC;
      break;
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
      stringId = IDS_TREE_DBALARM_DISCONN_SUCCESS_STATIC;
      break;
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
      stringId = IDS_TREE_DBALARM_DISCONN_FAILURE_STATIC;
      break;

    case OT_STATIC_R_CDB:
      stringId = IDS_TREE_R_CDB_STATIC;
      break;

    // level 1, child of "user"
    case OT_STATIC_R_USERSCHEMA:
      stringId = IDS_TREE_R_USERSCHEMA_STATIC;
      break;
    case OT_STATIC_R_USERGROUP:
      stringId = IDS_TREE_R_USERGROUP_STATIC;
      break;

    case OT_STATIC_R_SECURITY:
      stringId = IDS_TREE_R_SECURITY_STATIC;
      break;
    case OT_STATIC_R_SEC_SEL_SUCC:
      stringId = IDS_TREE_R_SEC_SEL_SUCC_STATIC;
      break;
    case OT_STATIC_R_SEC_SEL_FAIL:
      stringId = IDS_TREE_R_SEC_SEL_FAIL_STATIC;
      break;
    case OT_STATIC_R_SEC_DEL_SUCC:
      stringId = IDS_TREE_R_SEC_DEL_SUCC_STATIC;
      break;
    case OT_STATIC_R_SEC_DEL_FAIL:
      stringId = IDS_TREE_R_SEC_DEL_FAIL_STATIC;
      break;
    case OT_STATIC_R_SEC_INS_SUCC:
      stringId = IDS_TREE_R_SEC_INS_SUCC_STATIC;
      break;
    case OT_STATIC_R_SEC_INS_FAIL:
      stringId = IDS_TREE_R_SEC_INS_FAIL_STATIC;
      break;
    case OT_STATIC_R_SEC_UPD_SUCC:
      stringId = IDS_TREE_R_SEC_UPD_SUCC_STATIC;
      break;
    case OT_STATIC_R_SEC_UPD_FAIL:
      stringId = IDS_TREE_R_SEC_UPD_FAIL_STATIC;
      break;

    case OT_STATIC_R_GRANT:
      stringId = IDS_TREE_R_GRANT_STATIC;
      break;
    case OT_STATIC_R_DBEGRANT:
      stringId = IDS_TREE_R_DBEGRANT_STATIC;
      break;
    case OT_STATIC_R_DBEGRANT_RAISE:
      stringId = IDS_TREE_R_DBEGRANT_RAISE_STATIC;
      break;
    case OT_STATIC_R_DBEGRANT_REGISTER:
      stringId = IDS_TREE_R_DBEGRANT_REGISTER_STATIC;
      break;
    case OT_STATIC_R_PROCGRANT:
      stringId = IDS_TREE_R_PROCGRANT_STATIC;
      break;
    case OT_STATIC_R_PROCGRANT_EXEC:
      stringId = IDS_TREE_R_PROCGRANT_EXEC_STATIC;
      break;
    case OT_STATIC_R_SEQGRANT:
      stringId = IDS_TREE_R_SEQGRANT_STATIC;
      break;
    case OT_STATIC_R_SEQGRANT_NEXT:
      stringId = IDS_TREE_R_SEQGRANT_NEXT_STATIC;
      break;
    case OT_STATIC_R_DBGRANT:
      stringId = IDS_TREE_R_DBGRANT_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_ACCESY:
      stringId = IDS_TREE_R_DBGRANT_ACCESY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_ACCESN:
      stringId = IDS_TREE_R_DBGRANT_ACCESN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CREPRY:
      stringId = IDS_TREE_R_DBGRANT_CREPRY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CREPRN:
      stringId = IDS_TREE_R_DBGRANT_CREPRN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CRESEQY:
      stringId = IDS_TREE_R_DBGRANT_CRESEQY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CRESEQN:
      stringId = IDS_TREE_R_DBGRANT_CRESEQN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CRETBY:
      stringId = IDS_TREE_R_DBGRANT_CRETBY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CRETBN:
      stringId = IDS_TREE_R_DBGRANT_CRETBN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_DBADMY:
      stringId = IDS_TREE_R_DBGRANT_DBADMY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_DBADMN:
      stringId = IDS_TREE_R_DBGRANT_DBADMN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_LKMODY:
      stringId = IDS_TREE_R_DBGRANT_LKMODY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_LKMODN:
      stringId = IDS_TREE_R_DBGRANT_LKMODN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYIOY:
      stringId = IDS_TREE_R_DBGRANT_QRYIOY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYION:
      stringId = IDS_TREE_R_DBGRANT_QRYION_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYRWY:
      stringId = IDS_TREE_R_DBGRANT_QRYRWY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYRWN:
      stringId = IDS_TREE_R_DBGRANT_QRYRWN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_UPDSCY:
      stringId = IDS_TREE_R_DBGRANT_UPDSCY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_UPDSCN:
      stringId = IDS_TREE_R_DBGRANT_UPDSCN_STATIC;
      break;

    case OT_STATIC_R_DBGRANT_SELSCY:
      stringId = IDS_TREE_R_DBGRANT_SELSCY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_SELSCN:
      stringId = IDS_TREE_R_DBGRANT_SELSCN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CNCTLY:
      stringId = IDS_TREE_R_DBGRANT_CNCTLY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_CNCTLN:
      stringId = IDS_TREE_R_DBGRANT_CNCTLN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_IDLTLY:
      stringId = IDS_TREE_R_DBGRANT_IDLTLY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_IDLTLN:
      stringId = IDS_TREE_R_DBGRANT_IDLTLN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_SESPRY:
      stringId = IDS_TREE_R_DBGRANT_SESPRY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_SESPRN:
      stringId = IDS_TREE_R_DBGRANT_SESPRN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_TBLSTY:
      stringId = IDS_TREE_R_DBGRANT_TBLSTY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_TBLSTN:
      stringId = IDS_TREE_R_DBGRANT_TBLSTN_STATIC;
      break;

    case OT_STATIC_R_DBGRANT_QRYCPY:
      stringId = IDS_TREE_R_DBGRANT_QRYCPY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYCPN:
      stringId = IDS_TREE_R_DBGRANT_QRYCPN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYPGY:
      stringId = IDS_TREE_R_DBGRANT_QRYPGY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYPGN:
      stringId = IDS_TREE_R_DBGRANT_QRYPGN_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYCOY:
      stringId = IDS_TREE_R_DBGRANT_QRYCOY_STATIC;
      break;
    case OT_STATIC_R_DBGRANT_QRYCON:
      stringId = IDS_TREE_R_DBGRANT_QRYCON_STATIC;
      break;

    case OT_STATIC_R_TABLEGRANT:
      stringId = IDS_TREE_R_TABLEGRANT_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_SEL:
      stringId = IDS_TREE_R_TABLEGRANT_SEL_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_INS:
      stringId = IDS_TREE_R_TABLEGRANT_INS_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_UPD:
      stringId = IDS_TREE_R_TABLEGRANT_UPD_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_DEL:
      stringId = IDS_TREE_R_TABLEGRANT_DEL_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_REF:
      stringId = IDS_TREE_R_TABLEGRANT_REF_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_CPI:
      stringId = IDS_TREE_R_TABLEGRANT_CPI_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_CPF:
      stringId = IDS_TREE_R_TABLEGRANT_CPF_STATIC;
      break;
    case OT_STATIC_R_TABLEGRANT_ALL:
      stringId = IDS_TREE_R_TABLEGRANT_ALL_STATIC;
      break;
    case OT_STATIC_R_VIEWGRANT:
      stringId = IDS_TREE_R_VIEWGRANT_STATIC;
      break;
    case OT_STATIC_R_VIEWGRANT_SEL:
      stringId = IDS_TREE_R_VIEWGRANT_SEL_STATIC;
      break;
    case OT_STATIC_R_VIEWGRANT_INS:
      stringId = IDS_TREE_R_VIEWGRANT_INS_STATIC;
      break;
    case OT_STATIC_R_VIEWGRANT_UPD:
      stringId = IDS_TREE_R_VIEWGRANT_UPD_STATIC;
      break;
    case OT_STATIC_R_VIEWGRANT_DEL:
      stringId = IDS_TREE_R_VIEWGRANT_DEL_STATIC;
      break;
    case OT_STATIC_R_ROLEGRANT:
      stringId = IDS_TREE_R_ROLEGRANT_STATIC;
      break;

    // level 1, child of group
    case OT_STATIC_GROUPUSER:
      stringId = IDS_TREE_GROUPUSER_STATIC;
      break;

    // level 1, child of role
    case OT_STATIC_ROLEGRANT_USER:
      stringId = IDS_TREE_ROLEGRANT_USER_STATIC;
      break;

    // level 1, child of location
    case OT_STATIC_R_LOCATIONTABLE:
      stringId = IDS_TREE_R_LOCATIONTABLE_STATIC;
      break;

    // level 2, child of "table of database"
    case OT_STATIC_INTEGRITY:
      stringId = IDS_TREE_INTEGRITY_STATIC;
      break;
    case OT_STATIC_RULE:
      stringId = IDS_TREE_RULE_STATIC;
      break;
    case OT_STATIC_INDEX:
      stringId = IDS_TREE_INDEX_STATIC;
      break;
    case OT_STATIC_TABLELOCATION:
      stringId = IDS_TREE_TABLELOCATION_STATIC;
      break;
    case OT_STATIC_TABLEGRANTEES:
      stringId = IDS_TREE_TABLEGRANTEES_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_SEL_USER:
      stringId = IDS_TREE_TABLEGRANT_SEL_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_INS_USER:
      stringId = IDS_TREE_TABLEGRANT_INS_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_UPD_USER:
      stringId = IDS_TREE_TABLEGRANT_UPD_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_DEL_USER:
      stringId = IDS_TREE_TABLEGRANT_DEL_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_REF_USER:
      stringId = IDS_TREE_TABLEGRANT_REF_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_CPI_USER:
      stringId = IDS_TREE_TABLEGRANT_CPI_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_CPF_USER:
      stringId = IDS_TREE_TABLEGRANT_CPF_USER_STATIC;
      break;
    case OT_STATIC_TABLEGRANT_ALL_USER:
      stringId = IDS_TREE_TABLEGRANT_ALL_USER_STATIC;
      break;
    // desktop
    case OT_STATIC_TABLEGRANT_INDEX_USER:
      stringId = IDS_TREE_TABLEGRANT_INDEX_USER_STATIC;
      break;
    // desktop
    case OT_STATIC_TABLEGRANT_ALTER_USER:
      stringId = IDS_TREE_TABLEGRANT_ALTER_USER_STATIC;
      break;
    case OT_STATIC_SECURITY:
      stringId = IDS_TREE_SECURITY_STATIC;
      break;
    case OT_STATIC_SEC_SEL_SUCC:
      stringId = IDS_TREE_SEC_SEL_SUCC_STATIC;
      break;
    case OT_STATIC_SEC_SEL_FAIL:
      stringId = IDS_TREE_SEC_SEL_FAIL_STATIC;
      break;
    case OT_STATIC_SEC_DEL_SUCC:
      stringId = IDS_TREE_SEC_DEL_SUCC_STATIC;
      break;
    case OT_STATIC_SEC_DEL_FAIL:
      stringId = IDS_TREE_SEC_DEL_FAIL_STATIC;
      break;
    case OT_STATIC_SEC_INS_SUCC:
      stringId = IDS_TREE_SEC_INS_SUCC_STATIC;
      break;
    case OT_STATIC_SEC_INS_FAIL:
      stringId = IDS_TREE_SEC_INS_FAIL_STATIC;
      break;
    case OT_STATIC_SEC_UPD_SUCC:
      stringId = IDS_TREE_SEC_UPD_SUCC_STATIC;
      break;
    case OT_STATIC_SEC_UPD_FAIL:
      stringId = IDS_TREE_SEC_UPD_FAIL_STATIC;
      break;
    case OT_STATIC_R_TABLESYNONYM:
      stringId = IDS_TREE_R_TABLESYNONYM_STATIC;
      break;
    case OT_STATIC_R_TABLEVIEW:
      stringId = IDS_TREE_R_TABLEVIEW_STATIC;
      break;

    // level 2, child of "view of database"
    case OT_STATIC_VIEWTABLE:
      stringId = IDS_TREE_VIEWTABLE_STATIC;
      break;
    case OT_STATIC_VIEWGRANTEES:
      stringId = IDS_TREE_VIEWGRANTEES_STATIC;
      break;
    case OT_STATIC_VIEWGRANT_SEL_USER:
      stringId = IDS_TREE_VIEWGRANT_SEL_USER_STATIC;
      break;
    case OT_STATIC_VIEWGRANT_INS_USER:
      stringId = IDS_TREE_VIEWGRANT_INS_USER_STATIC;
      break;
    case OT_STATIC_VIEWGRANT_UPD_USER:
      stringId = IDS_TREE_VIEWGRANT_UPD_USER_STATIC;
      break;
    case OT_STATIC_VIEWGRANT_DEL_USER:
      stringId = IDS_TREE_VIEWGRANT_DEL_USER_STATIC;
      break;
    case OT_STATIC_R_VIEWSYNONYM:
      stringId = IDS_TREE_R_VIEWSYNONYM_STATIC;
      break;

    // level 2, child of "procedure of database"
    case OT_STATIC_PROCGRANT_EXEC_USER:
      stringId = IDS_TREE_PROCGRANT_EXEC_USER_STATIC;
      break;
    case OT_STATIC_R_PROC_RULE:
      stringId = IDS_TREE_R_PROC_RULE_STATIC;
      break;

    // level 2, child of "Sequence of database"
    case OT_STATIC_SEQGRANT_NEXT_USER:
      stringId = IDS_TREE_SEQUENCE_GRANT_STATIC;
      break;

    // level 2, child of "dbevent of database"
    case OT_STATIC_DBEGRANT_RAISE_USER:
      stringId = IDS_TREE_DBEGRANT_RAISE_USER_STATIC;
      break;
    case OT_STATIC_DBEGRANT_REGTR_USER:
      stringId = IDS_TREE_DBEGRANT_REGTR_USER_STATIC;
      break;

    // level 2, child of "schemauser on database"
    case OT_STATIC_SCHEMAUSER_TABLE:
      stringId = IDS_TREE_SCHEMAUSER_TABLE_STATIC;
      break;
    case OT_STATIC_SCHEMAUSER_VIEW:
      stringId = IDS_TREE_SCHEMAUSER_VIEW_STATIC;
      break;
    case OT_STATIC_SCHEMAUSER_PROCEDURE:
      stringId = IDS_TREE_SCHEMAUSER_PROCEDURE_STATIC;
      break;

    // level 3, child of "index on table of database"
    case OT_STATIC_R_INDEXSYNONYM:
      stringId = IDS_TREE_R_INDEXSYNONYM_STATIC;
      break;

    // level 3, child of "rule on table of database"
    case OT_STATIC_RULEPROC:
      stringId = IDS_TREE_RULEPROC_STATIC;
      break;

    // replicator, all levels mixed
    case OT_STATIC_REPLICATOR:
      stringId = IDS_TREE_REPLICATOR_STATIC;
      break;

    case OT_STATIC_REPLIC_CONNECTION:
      stringId = IDS_TREE_REPLIC_CONNECTION_STATIC;
      break;
    case OT_STATIC_REPLIC_CDDS:
      stringId = IDS_TREE_REPLIC_CDDS_STATIC;
      break;
    case OT_STATIC_REPLIC_MAILUSER:
      stringId = IDS_TREE_REPLIC_MAILUSER_STATIC;
      break;
    case OT_STATIC_REPLIC_REGTABLE:
      stringId = IDS_TREE_REPLIC_REGTABLE_STATIC;
      break;
    case OT_STATIC_REPLIC_CDDS_DETAIL:
      stringId = IDS_TREE_REPLIC_CDDS_DETAIL_STATIC;
      break;
    case OT_STATIC_R_REPLIC_CDDS_TABLE:
      stringId = IDS_TREE_R_REPLIC_CDDS_TABLE_STATIC;
      break;

    case OT_STATIC_R_REPLIC_TABLE_CDDS:
      stringId = IDS_TREE_R_REPLIC_TABLE_CDDS_STATIC;
      break;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_STATIC_ALARMEE:
      stringId = IDS_TREE_ALARMEE_STATIC;
      break;
    case OT_STATIC_ALARM_EVENT:
      stringId = IDS_TREE_ALARM_EVENT_STATIC;
      break;

    //
    // ICE
    //
    case OT_STATIC_ICE                      :
      stringId = IDS_TREE_ICE_STATIC;
      break;
    // Under "Security"
    case OT_STATIC_ICE_SECURITY             :
      stringId = IDS_TREE_ICE_SECURITY_STATIC;
      break;
    case OT_STATIC_ICE_ROLE               :
      stringId = IDS_TREE_ICE_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_DBUSER               :
      stringId = IDS_TREE_ICE_DBUSER_STATIC;
      break;
    case OT_STATIC_ICE_DBCONNECTION         :
      stringId = IDS_TREE_ICE_DBCONNECTION_STATIC;
      break;
    case OT_STATIC_ICE_WEBUSER              :
      stringId = IDS_TREE_ICE_WEBUSER_STATIC;
      break;
    case OT_STATIC_ICE_WEBUSER_ROLE         :
      stringId = IDS_TREE_ICE_WEBUSER_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_WEBUSER_CONNECTION   :
      stringId = IDS_TREE_ICE_WEBUSER_CONNECTION_STATIC;
      break;
    case OT_STATIC_ICE_PROFILE              :
      stringId = IDS_TREE_ICE_PROFILE_STATIC;
      break;
    case OT_STATIC_ICE_PROFILE_ROLE         :
      stringId = IDS_TREE_ICE_PROFILE_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_PROFILE_CONNECTION   :
      stringId = IDS_TREE_ICE_PROFILE_CONNECTION_STATIC;
      break;
    // Under "Bussiness unit" (BUNIT)
    case OT_STATIC_ICE_BUNIT                :
      stringId = IDS_TREE_ICE_BUNIT_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_SECURITY       :
      stringId = IDS_TREE_ICE_BUNIT_SECURITY_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_SEC_ROLE       :
      stringId = IDS_TREE_ICE_BUNIT_SEC_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_SEC_USER       :
      stringId = IDS_TREE_ICE_BUNIT_SEC_USER_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_FACET          :
      stringId = IDS_TREE_ICE_BUNIT_FACET_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE           :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_FACET_ROLE     :
      stringId = IDS_TREE_ICE_BUNIT_FACET_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_FACET_USER     :
      stringId = IDS_TREE_ICE_BUNIT_FACET_USER_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE     :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_ROLE_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_USER     :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_USER_STATIC;
      break;
    case OT_STATIC_ICE_BUNIT_LOCATION       :
      stringId = IDS_TREE_ICE_BUNIT_LOCATION_STATIC;
      break;
    // Under "Server"
    case OT_STATIC_ICE_SERVER               :
      stringId = IDS_TREE_ICE_SERVER_STATIC;
      break;
    case OT_STATIC_ICE_SERVER_APPLICATION   :
      stringId = IDS_TREE_ICE_SERVER_APPLICATION_STATIC;
      break;
    case OT_STATIC_ICE_SERVER_LOCATION      :
      stringId = IDS_TREE_ICE_SERVER_LOCATION_STATIC;
      break;
    case OT_STATIC_ICE_SERVER_VARIABLE      :
      stringId = IDS_TREE_ICE_SERVER_VARIABLE_STATIC;
      break;

    //
    // INSTALLATION LEVEL SETTINGS
    //
    case OT_STATIC_INSTALL                  :
      stringId = IDS_TREE_INSTALL_STATIC;
      break;
    case OT_STATIC_INSTALL_SECURITY         :
      stringId = IDS_TREE_INSTALL_SECURITY_STATIC;
      break;
    case OT_STATIC_INSTALL_GRANTEES         :
      stringId = IDS_TREE_INSTALL_GRANTEES_STATIC;
      break;
    case OT_STATIC_INSTALL_ALARMS           :
      stringId = IDS_TREE_INSTALL_ALARMS_STATIC;
      break;

    default:
      stringId = 0;
      break;
  }
  return stringId;
}

//
// Returns the "object name" string id for a static type,
// used to display in the tree line
//
UINT GetStaticStatusStringId(int staticType)
{
  UINT  stringId;

  switch (staticType) {
    // level 0
    case OT_STATIC_DATABASE:
      stringId = IDS_DATABASE_STATIC_STATUS;
      break;
    case OT_STATIC_PROFILE:
      stringId = IDS_PROFILE_STATIC_STATUS;
      break;
    case OT_STATIC_USER:
      stringId = IDS_USER_STATIC_STATUS;
      break;
    case OT_STATIC_GROUP:
      stringId = IDS_GROUP_STATIC_STATUS;
      break;
    case OT_STATIC_ROLE:
      stringId = IDS_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_LOCATION:
      stringId = IDS_LOCATION_STATIC_STATUS;
      break;
    case OT_STATIC_SYNONYMED:
      stringId = IDS_SYNONYMED_STATIC_STATUS;
      break;

    // level 1, child of database
    case OT_STATIC_TABLE:
      stringId = IDS_TABLE_STATIC_STATUS;
      break;
    case OT_STATIC_VIEW:
      stringId = IDS_VIEW_STATIC_STATUS;
      break;
    case OT_STATIC_PROCEDURE:
      stringId = IDS_PROCEDURE_STATIC_STATUS;
      break;
    case OT_STATIC_SEQUENCE:
      stringId = IDS_TREE_SEQUENCE_STATIC_STATUS;
      break;
    case OT_STATIC_SCHEMA:
      stringId = IDS_SCHEMA_STATIC_STATUS;
      break;
    case OT_STATIC_SYNONYM:
      stringId = IDS_SYNONYM_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES:
      stringId = IDS_DBGRANTEES_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_ACCESY:
      stringId = IDS_DBGRANTEES_ACCESY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_ACCESN:
      stringId = IDS_DBGRANTEES_ACCESN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CREPRY:
      stringId = IDS_DBGRANTEES_CREPRY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CREPRN:
      stringId = IDS_DBGRANTEES_CREPRN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CRETBY:
      stringId = IDS_DBGRANTEES_CRETBY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CRETBN:
      stringId = IDS_DBGRANTEES_CRETBN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_DBADMY:
      stringId = IDS_DBGRANTEES_DBADMY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_DBADMN:
      stringId = IDS_DBGRANTEES_DBADMN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_LKMODY:
      stringId = IDS_DBGRANTEES_LKMODY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_LKMODN:
      stringId = IDS_DBGRANTEES_LKMODN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYIOY:
      stringId = IDS_DBGRANTEES_QRYIOY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYION:
      stringId = IDS_DBGRANTEES_QRYION_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYRWY:
      stringId = IDS_DBGRANTEES_QRYRWY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYRWN:
      stringId = IDS_DBGRANTEES_QRYRWN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_UPDSCY:
      stringId = IDS_DBGRANTEES_UPDSCY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_UPDSCN:
      stringId = IDS_DBGRANTEES_UPDSCN_STATIC_STATUS;
      break;

    case OT_STATIC_DBGRANTEES_SELSCY:
      stringId = IDS_DBGRANTEES_SELSCY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_SELSCN:
      stringId = IDS_DBGRANTEES_SELSCN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CNCTLY:
      stringId = IDS_DBGRANTEES_CNCTLY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CNCTLN:
      stringId = IDS_DBGRANTEES_CNCTLN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_IDLTLY:
      stringId = IDS_DBGRANTEES_IDLTLY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_IDLTLN:
      stringId = IDS_DBGRANTEES_IDLTLN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_SESPRY:
      stringId = IDS_DBGRANTEES_SESPRY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_SESPRN:
      stringId = IDS_DBGRANTEES_SESPRN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_TBLSTY:
      stringId = IDS_DBGRANTEES_TBLSTY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_TBLSTN:
      stringId = IDS_DBGRANTEES_TBLSTN_STATIC_STATUS;
      break;

    case OT_STATIC_DBGRANTEES_QRYCPY:
      stringId = IDS_DBGRANTEES_QRYCPY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYCPN:
      stringId = IDS_DBGRANTEES_QRYCPN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYPGY:
      stringId = IDS_DBGRANTEES_QRYPGY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYPGN:
      stringId = IDS_DBGRANTEES_QRYPGN_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYCOY:
      stringId = IDS_DBGRANTEES_QRYCOY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_QRYCON:
      stringId = IDS_DBGRANTEES_QRYCON_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQY:
      stringId = IDS_DBGRANTEES_CRSEQY_STATIC_STATUS;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQN:
      stringId = IDS_DBGRANTEES_CRSEQN_STATIC_STATUS;
      break;

    case OT_STATIC_DBEVENT:
      stringId = IDS_DBEVENT_STATIC_STATUS;
      break;

    case OT_STATIC_DBALARM:
      stringId = IDS_DBALARM_STATIC_STATUS;
      break;
    case OT_STATIC_DBALARM_CONN_SUCCESS:
      stringId = IDS_DBALARM_CONN_SUCCESS_STATIC_STATUS;
      break;
    case OT_STATIC_DBALARM_CONN_FAILURE:
      stringId = IDS_DBALARM_CONN_FAILURE_STATIC_STATUS;
      break;
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
      stringId = IDS_DBALARM_DISCONN_SUCCESS_STATIC_STATUS;
      break;
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
      stringId = IDS_DBALARM_DISCONN_FAILURE_STATIC_STATUS;
      break;

    case OT_STATIC_R_CDB:
      stringId = IDS_TREE_R_CDB_STATIC_STATUS;
      break;

    // level 1, child of "user"
    case OT_STATIC_R_USERSCHEMA:
      stringId = IDS_R_USERSCHEMA_STATIC_STATUS;
      break;
    case OT_STATIC_R_USERGROUP:
      stringId = IDS_R_USERGROUP_STATIC_STATUS;
      break;

    case OT_STATIC_R_SECURITY:
      stringId = IDS_R_SECURITY_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_SEL_SUCC:
      stringId = IDS_R_SEC_SEL_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_SEL_FAIL:
      stringId = IDS_R_SEC_SEL_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_DEL_SUCC:
      stringId = IDS_R_SEC_DEL_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_DEL_FAIL:
      stringId = IDS_R_SEC_DEL_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_INS_SUCC:
      stringId = IDS_R_SEC_INS_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_INS_FAIL:
      stringId = IDS_R_SEC_INS_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_UPD_SUCC:
      stringId = IDS_R_SEC_UPD_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEC_UPD_FAIL:
      stringId = IDS_R_SEC_UPD_FAIL_STATIC_STATUS;
      break;

    case OT_STATIC_R_GRANT:
      stringId = IDS_R_GRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBEGRANT:
      stringId = IDS_R_DBEGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBEGRANT_RAISE:
      stringId = IDS_R_DBEGRANT_RAISE_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBEGRANT_REGISTER:
      stringId = IDS_R_DBEGRANT_REGISTER_STATIC_STATUS;
      break;
    case OT_STATIC_R_PROCGRANT:
      stringId = IDS_R_PROCGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_PROCGRANT_EXEC:
      stringId = IDS_R_PROCGRANT_EXEC_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEQGRANT:
      stringId = IDS_R_SEQGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_SEQGRANT_NEXT:
      stringId = IDS_R_SEQGRANT_NEXT_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT:
      stringId = IDS_R_DBGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_ACCESY:
      stringId = IDS_R_DBGRANT_ACCESY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_ACCESN:
      stringId = IDS_R_DBGRANT_ACCESN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CREPRY:
      stringId = IDS_R_DBGRANT_CREPRY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CREPRN:
      stringId = IDS_R_DBGRANT_CREPRN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CRESEQY:
      stringId = IDS_R_DBGRANT_CRESEQY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CRESEQN:
      stringId = IDS_R_DBGRANT_CRESEQN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CRETBY:
      stringId = IDS_R_DBGRANT_CRETBY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CRETBN:
      stringId = IDS_R_DBGRANT_CRETBN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_DBADMY:
      stringId = IDS_R_DBGRANT_DBADMY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_DBADMN:
      stringId = IDS_R_DBGRANT_DBADMN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_LKMODY:
      stringId = IDS_R_DBGRANT_LKMODY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_LKMODN:
      stringId = IDS_R_DBGRANT_LKMODN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYIOY:
      stringId = IDS_R_DBGRANT_QRYIOY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYION:
      stringId = IDS_R_DBGRANT_QRYION_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYRWY:
      stringId = IDS_R_DBGRANT_QRYRWY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYRWN:
      stringId = IDS_R_DBGRANT_QRYRWN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_UPDSCY:
      stringId = IDS_R_DBGRANT_UPDSCY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_UPDSCN:
      stringId = IDS_R_DBGRANT_UPDSCN_STATIC_STATUS;
      break;

    case OT_STATIC_R_DBGRANT_SELSCY:
      stringId = IDS_R_DBGRANT_SELSCY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_SELSCN:
      stringId = IDS_R_DBGRANT_SELSCN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CNCTLY:
      stringId = IDS_R_DBGRANT_CNCTLY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_CNCTLN:
      stringId = IDS_R_DBGRANT_CNCTLN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_IDLTLY:
      stringId = IDS_R_DBGRANT_IDLTLY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_IDLTLN:
      stringId = IDS_R_DBGRANT_IDLTLN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_SESPRY:
      stringId = IDS_R_DBGRANT_SESPRY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_SESPRN:
      stringId = IDS_R_DBGRANT_SESPRN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_TBLSTY:
      stringId = IDS_R_DBGRANT_TBLSTY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_TBLSTN:
      stringId = IDS_R_DBGRANT_TBLSTN_STATIC_STATUS;
      break;

    case OT_STATIC_R_DBGRANT_QRYCPY:
      stringId = IDS_R_DBGRANT_QRYCPY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYCPN:
      stringId = IDS_R_DBGRANT_QRYCPN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYPGY:
      stringId = IDS_R_DBGRANT_QRYPGY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYPGN:
      stringId = IDS_R_DBGRANT_QRYPGN_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYCOY:
      stringId = IDS_R_DBGRANT_QRYCOY_STATIC_STATUS;
      break;
    case OT_STATIC_R_DBGRANT_QRYCON:
      stringId = IDS_R_DBGRANT_QRYCON_STATIC_STATUS;
      break;

    case OT_STATIC_R_TABLEGRANT:
      stringId = IDS_R_TABLEGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_SEL:
      stringId = IDS_R_TABLEGRANT_SEL_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_INS:
      stringId = IDS_R_TABLEGRANT_INS_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_UPD:
      stringId = IDS_R_TABLEGRANT_UPD_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_DEL:
      stringId = IDS_R_TABLEGRANT_DEL_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_REF:
      stringId = IDS_R_TABLEGRANT_REF_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_CPI:
      stringId = IDS_R_TABLEGRANT_CPI_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_CPF:
      stringId = IDS_R_TABLEGRANT_CPF_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEGRANT_ALL:
      stringId = IDS_R_TABLEGRANT_ALL_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWGRANT:
      stringId = IDS_R_VIEWGRANT_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWGRANT_SEL:
      stringId = IDS_R_VIEWGRANT_SEL_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWGRANT_INS:
      stringId = IDS_R_VIEWGRANT_INS_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWGRANT_UPD:
      stringId = IDS_R_VIEWGRANT_UPD_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWGRANT_DEL:
      stringId = IDS_R_VIEWGRANT_DEL_STATIC_STATUS;
      break;
    case OT_STATIC_R_ROLEGRANT:
      stringId = IDS_R_ROLEGRANT_STATIC_STATUS;
      break;

    // level 1, child of group
    case OT_STATIC_GROUPUSER:
      stringId = IDS_GROUPUSER_STATIC_STATUS;
      break;

    // level 1, child of role
    case OT_STATIC_ROLEGRANT_USER:
      stringId = IDS_ROLEGRANT_USER_STATIC_STATUS;
      break;

    // level 1, child of location
    case OT_STATIC_R_LOCATIONTABLE:
      stringId = IDS_R_LOCATIONTABLE_STATIC_STATUS;
      break;

    // level 2, child of "table of database"
    case OT_STATIC_INTEGRITY:
      stringId = IDS_INTEGRITY_STATIC_STATUS;
      break;
    case OT_STATIC_RULE:
      stringId = IDS_RULE_STATIC_STATUS;
      break;
    case OT_STATIC_INDEX:
      stringId = IDS_INDEX_STATIC_STATUS;
      break;
    case OT_STATIC_TABLELOCATION:
      stringId = IDS_TABLELOCATION_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANTEES:
      stringId = IDS_TABLEGRANTEES_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_SEL_USER:
      stringId = IDS_TABLEGRANT_SEL_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_INS_USER:
      stringId = IDS_TABLEGRANT_INS_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_UPD_USER:
      stringId = IDS_TABLEGRANT_UPD_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_DEL_USER:
      stringId = IDS_TABLEGRANT_DEL_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_REF_USER:
      stringId = IDS_TABLEGRANT_REF_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_CPI_USER:
      stringId = IDS_TABLEGRANT_CPI_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_CPF_USER:
      stringId = IDS_TABLEGRANT_CPF_USER_STATIC_STATUS;
      break;
    case OT_STATIC_TABLEGRANT_ALL_USER:
      stringId = IDS_TABLEGRANT_ALL_USER_STATIC_STATUS;
      break;
    // desktop
    case OT_STATIC_TABLEGRANT_INDEX_USER:
      stringId = IDS_TABLEGRANT_INDEX_USER_STATIC_STATUS;
      break;
    // desktop
    case OT_STATIC_TABLEGRANT_ALTER_USER:
      stringId = IDS_TABLEGRANT_ALTER_USER_STATIC_STATUS;
      break;
    case OT_STATIC_SECURITY:
      stringId = IDS_SECURITY_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_SEL_SUCC:
      stringId = IDS_SEC_SEL_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_SEL_FAIL:
      stringId = IDS_SEC_SEL_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_DEL_SUCC:
      stringId = IDS_SEC_DEL_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_DEL_FAIL:
      stringId = IDS_SEC_DEL_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_INS_SUCC:
      stringId = IDS_SEC_INS_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_INS_FAIL:
      stringId = IDS_SEC_INS_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_UPD_SUCC:
      stringId = IDS_SEC_UPD_SUCC_STATIC_STATUS;
      break;
    case OT_STATIC_SEC_UPD_FAIL:
      stringId = IDS_SEC_UPD_FAIL_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLESYNONYM:
      stringId = IDS_R_TABLESYNONYM_STATIC_STATUS;
      break;
    case OT_STATIC_R_TABLEVIEW:
      stringId = IDS_R_TABLEVIEW_STATIC_STATUS;
      break;

    // level 2, child of "view of database"
    case OT_STATIC_VIEWTABLE:
      stringId = IDS_VIEWTABLE_STATIC_STATUS;
      break;
    case OT_STATIC_VIEWGRANTEES:
      stringId = IDS_VIEWGRANTEES_STATIC_STATUS;
      break;
    case OT_STATIC_VIEWGRANT_SEL_USER:
      stringId = IDS_VIEWGRANT_SEL_USER_STATIC_STATUS;
      break;
    case OT_STATIC_VIEWGRANT_INS_USER:
      stringId = IDS_VIEWGRANT_INS_USER_STATIC_STATUS;
      break;
    case OT_STATIC_VIEWGRANT_UPD_USER:
      stringId = IDS_VIEWGRANT_UPD_USER_STATIC_STATUS;
      break;
    case OT_STATIC_VIEWGRANT_DEL_USER:
      stringId = IDS_VIEWGRANT_DEL_USER_STATIC_STATUS;
      break;
    case OT_STATIC_R_VIEWSYNONYM:
      stringId = IDS_R_VIEWSYNONYM_STATIC_STATUS;
      break;

    // level 2, child of "procedure of database"
    case OT_STATIC_PROCGRANT_EXEC_USER:
      stringId = IDS_PROCGRANT_EXEC_USER_STATIC_STATUS;
      break;
    case OT_STATIC_R_PROC_RULE:
      stringId = IDS_R_PROC_RULE_STATIC_STATUS;
      break;

    // level 2, child of "Sequence of database"
    case OT_STATIC_SEQGRANT_NEXT_USER:
      stringId = IDS_TREE_SEQUENCE_GRANT_STATIC_STATUS;
      break;

    // level 2, child of "dbevent of database"
    case OT_STATIC_DBEGRANT_RAISE_USER:
      stringId = IDS_DBEGRANT_RAISE_USER_STATIC_STATUS;
      break;
    case OT_STATIC_DBEGRANT_REGTR_USER:
      stringId = IDS_DBEGRANT_REGTR_USER_STATIC_STATUS;
      break;

    // level 2, child of "schemauser on database"
    case OT_STATIC_SCHEMAUSER_TABLE:
      stringId = IDS_SCHEMAUSER_TABLE_STATIC_STATUS;
      break;
    case OT_STATIC_SCHEMAUSER_VIEW:
      stringId = IDS_SCHEMAUSER_VIEW_STATIC_STATUS;
      break;
    case OT_STATIC_SCHEMAUSER_PROCEDURE:
      stringId = IDS_SCHEMAUSER_PROCEDURE_STATIC_STATUS;
      break;

    // level 3, child of "index on table of database"
    case OT_STATIC_R_INDEXSYNONYM:
      stringId = IDS_R_INDEXSYNONYM_STATIC_STATUS;
      break;

    // level 3, child of "rule on table of database"
    case OT_STATIC_RULEPROC:
      stringId = IDS_RULEPROC_STATIC_STATUS;
      break;

    // replicator, all levels mixed
    case OT_STATIC_REPLICATOR:
      stringId = IDS_REPLICATOR_STATIC_STATUS;
      break;

    case OT_STATIC_REPLIC_CONNECTION:
      stringId = IDS_REPLIC_CONNECTION_STATIC_STATUS;
      break;
    case OT_STATIC_REPLIC_CDDS:
      stringId = IDS_REPLIC_CDDS_STATIC_STATUS;
      break;
    case OT_STATIC_REPLIC_MAILUSER:
      stringId = IDS_REPLIC_MAILUSER_STATIC_STATUS;
      break;
    case OT_STATIC_REPLIC_REGTABLE:
      stringId = IDS_REPLIC_REGTABLE_STATIC_STATUS;
      break;

    case OT_STATIC_REPLIC_CDDS_DETAIL:
      stringId = IDS_REPLIC_CDDS_DETAIL_STATIC_STATUS;
      break;
    case OT_STATIC_R_REPLIC_CDDS_TABLE:
      stringId = IDS_R_REPLIC_CDDS_TABLE_STATIC_STATUS;
      break;

    case OT_STATIC_R_REPLIC_TABLE_CDDS:
      stringId = IDS_R_REPLIC_TABLE_CDDS_STATIC_STATUS;
      break;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_STATIC_ALARMEE:
      stringId = IDS_ALARMEE_STATIC_STATUS;
      break;
    case OT_STATIC_ALARM_EVENT:
      stringId = IDS_ALARM_EVENT_STATIC_STATUS;
      break;

    //
    // ICE
    //
    case OT_STATIC_ICE                      :
      stringId = IDS_TREE_ICE_STATIC_STATUS;
      break;
    // Under "Security"
    case OT_STATIC_ICE_SECURITY             :
      stringId = IDS_TREE_ICE_SECURITY_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_ROLE               :
      stringId = IDS_TREE_ICE_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_DBUSER               :
      stringId = IDS_TREE_ICE_DBUSER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_DBCONNECTION         :
      stringId = IDS_TREE_ICE_DBCONNECTION_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_WEBUSER              :
      stringId = IDS_TREE_ICE_WEBUSER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_WEBUSER_ROLE         :
      stringId = IDS_TREE_ICE_WEBUSER_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_WEBUSER_CONNECTION   :
      stringId = IDS_TREE_ICE_WEBUSER_CONNECTION_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_PROFILE              :
      stringId = IDS_TREE_ICE_PROFILE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_PROFILE_ROLE         :
      stringId = IDS_TREE_ICE_PROFILE_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_PROFILE_CONNECTION   :
      stringId = IDS_TREE_ICE_PROFILE_CONNECTION_STATIC_STATUS;
      break;
    // Under "Bussiness unit" (BUNIT)
    case OT_STATIC_ICE_BUNIT                :
      stringId = IDS_TREE_ICE_BUNIT_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_SECURITY       :
      stringId = IDS_TREE_ICE_BUNIT_SECURITY_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_SEC_ROLE       :
      stringId = IDS_TREE_ICE_BUNIT_SEC_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_SEC_USER       :
      stringId = IDS_TREE_ICE_BUNIT_SEC_USER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_FACET          :
      stringId = IDS_TREE_ICE_BUNIT_FACET_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE           :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_FACET_ROLE     :
      stringId = IDS_TREE_ICE_BUNIT_FACET_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_FACET_USER     :
      stringId = IDS_TREE_ICE_BUNIT_FACET_USER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE     :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_ROLE_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_PAGE_USER     :
      stringId = IDS_TREE_ICE_BUNIT_PAGE_USER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_BUNIT_LOCATION       :
      stringId = IDS_TREE_ICE_BUNIT_LOCATION_STATIC_STATUS;
      break;
    // Under "Server"
    case OT_STATIC_ICE_SERVER               :
      stringId = IDS_TREE_ICE_SERVER_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_SERVER_APPLICATION   :
      stringId = IDS_TREE_ICE_SERVER_APPLICATION_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_SERVER_LOCATION      :
      stringId = IDS_TREE_ICE_SERVER_LOCATION_STATIC_STATUS;
      break;
    case OT_STATIC_ICE_SERVER_VARIABLE      :
      stringId = IDS_TREE_ICE_SERVER_VARIABLE_STATIC_STATUS;
      break;

    //
    // INSTALLATION LEVEL SETTINGS
    //
    case OT_STATIC_INSTALL                  :
      stringId = IDS_TREE_INSTALL_STATIC_STATUS;
      break;
    case OT_STATIC_INSTALL_SECURITY         :
      stringId = IDS_TREE_INSTALL_SECURITY_STATIC_STATUS;
      break;
    case OT_STATIC_INSTALL_GRANTEES         :
      stringId = IDS_TREE_INSTALL_GRANTEES_STATIC_STATUS;
      break;
    case OT_STATIC_INSTALL_ALARMS           :
      stringId = IDS_TREE_INSTALL_ALARMS_STATIC_STATUS;
      break;

    default:
      // forgotten string: use same as full length static obj name
      stringId = GetStaticStringId(staticType);
      break;
  }
  return stringId;
}

//
//  Dummy record creation
//
DWORD AddDummySubItem(LPDOMDATA lpDomData, DWORD idParent)
{
  DWORD         dret;
  UCHAR         buf[MAXOBJECTNAME];
  LPTREERECORD  lpRecord;

  LoadString(hResource, IDS_TREE_DUMMY, buf, sizeof(buf));
  lpRecord = AllocAndFillRecord(OT_STATIC_DUMMY, FALSE, NULL, NULL, NULL, 0,
                                buf, NULL, NULL, NULL);
  if (!lpRecord)
    return 0;   // Error!
  dret = TreeAddRecord(lpDomData, buf, idParent, 0, 0, lpRecord);
  return dret;
}

//
//  Problem-indicating record creation
//
//  Manages RES_ERR, RES_TIMEOUT, RES_NOGRANT
//  New error types should be added here
//
//  RES_ENDOFDATA and RES_ALREADYEXIST not concerned with
//
//  Modified 11/5/95 : new parameters to create the record
//  with the object type and the parenthood,
//  in order to make the menuitem "add object" work.
//
DWORD AddProblemSubItem(LPDOMDATA lpDomData, DWORD idParent, int errortype, int iobjecttype, LPTREERECORD lpRefRecord)
{
  DWORD         dret;
  UCHAR         buf[MAXOBJECTNAME];
  LPTREERECORD  lpRecord;
  UINT          errorStringId;
  //int           errorObjType;

  switch (errortype) {
    case RES_NOGRANT:
      errorStringId = IDS_TREE_ERR_NOGRANT;
      //errorObjType  = OT_STATIC_ERR_NOGRANT;
      break;
    case RES_TIMEOUT:
      errorStringId = IDS_TREE_ERR_TIMEOUT;
      //errorObjType  = OT_STATIC_ERR_TIMEOUT;
      break;
    case RES_ERR:
      errorStringId = IDS_TREE_ERROR;
      //errorObjType  = OT_STATIC_ERROR;
    default:
      break;
  }
  //LoadString(hResource, errorStringId, buf, sizeof(buf));
  //lpRecord = AllocAndFillRecord(errorObjType, FALSE, NULL, NULL, NULL,
  //                              buf, NULL, NULL, NULL);

  buf[0] = '\0';    // empty object name, for IsNoItem() in domsplit.c
  lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                lpRefRecord->extra,
                                lpRefRecord->extra2,
                                lpRefRecord->extra3,
                                lpRefRecord->parentDbType,
                                buf, NULL, NULL, NULL);
  if (!lpRecord)
    return 0;   // Error!
  LoadString(hResource, errorStringId, buf, sizeof(buf));
  dret = TreeAddRecord(lpDomData, buf, idParent, 0, 0, lpRecord);
  return dret;
}

//
//  Find the next id of a line in the tree starting from previousId
//  matching the requested type and parentstrings,
//  or the first if previousId is 0L
//
//  IMPORTANT : aparenstrings is assumed not null
//
//  returns 0L if no more line found
//
//  if necessary, returns parentage data in bufPar1 and bufPar2
//  which will be used to build a temporary aparents fully filled
//
//  receives a bool saying whether we should try the received idItem
//  (set to true if the previous anchor was a root item and was deleted)
//
//  receives a pointer on a bool "pbRootItem" to be filled to indicate
//  whether the found anchor is a root item.
//
DWORD GetAnchorId(LPDOMDATA lpDomData, int iobjecttype, int level, LPUCHAR *parentstrings, DWORD previousId, LPUCHAR bufPar1, LPUCHAR bufPar2, LPUCHAR bufPar3, int iAction, DWORD idItem, BOOL bTryCurrent, BOOL *pbRootItem)
{
  DWORD         id;
  LPTREERECORD  lpRecord;
  int           recType;

  // first of all
  bufPar1[0] = '\0';
  bufPar2[0] = '\0';
  bufPar3[0] = '\0';

  // default value for root item flag
  *pbRootItem = FALSE;

  // manage special actions
  if (iAction == ACT_EXPANDITEM)
    if (!previousId)
      return idItem;      // first anchor : item that generated the expansion
    else
      return 0L;          // only one anchor if expanditem

  // Zero levels of parenthood
  switch (iobjecttype) {
    case OT_DATABASE:
      recType  = OT_STATIC_DATABASE;
      break;
    case OT_PROFILE:
      recType  = OT_STATIC_PROFILE;
      break;
    case OT_USER:
      recType  = OT_STATIC_USER;
      break;
    case OT_GROUP:
      recType  = OT_STATIC_GROUP;
      break;
    case OT_ROLE:
      recType  = OT_STATIC_ROLE;
      break;
    case OT_LOCATION:
      recType  = OT_STATIC_LOCATION;
      break;

    // ICE types, level 0
    case OT_ICE_MAIN           :
    case OT_ICE_SUBSTATICS           :
      recType = OT_STATIC_ICE;
      break;
    case OT_ICE_ROLE               :
      recType = OT_STATIC_ICE_ROLE;
      break;
    case OT_ICE_DBUSER               :
      recType = OT_STATIC_ICE_DBUSER;
      break;
    case OT_ICE_DBCONNECTION         :
      recType = OT_STATIC_ICE_DBCONNECTION;
      break;
    case OT_ICE_SERVER_APPLICATION   :
      recType = OT_STATIC_ICE_SERVER_APPLICATION;
      break;
    case OT_ICE_SERVER_LOCATION      :
      recType = OT_STATIC_ICE_SERVER_LOCATION;
      break;
    case OT_ICE_SERVER_VARIABLE      :
      recType = OT_STATIC_ICE_SERVER_VARIABLE;
      break;
    case OT_ICE_WEBUSER              :
      recType = OT_STATIC_ICE_WEBUSER;
      break;
    case OT_ICE_PROFILE              :
      recType = OT_STATIC_ICE_PROFILE;
      break;
    case OT_ICE_BUNIT                :
      recType = OT_STATIC_ICE_BUNIT;
      break;

    default:
      recType = 0;
  }
  if (recType) {
    if (previousId)
      if (bTryCurrent)
        id = previousId;
      else
        id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                                                0, (LPARAM)previousId);
    else
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
    while (id) {
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)id);
      if (lpRecord->recType == recType)
        return id;

      // manage root item
      if (lpRecord->recType == iobjecttype)
        if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                        0, (LPARAM)id) == 0) {
          *pbRootItem = TRUE;
          return id;
        }

      id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
    }
    return 0;
  }

  // One levels of parenthood
  switch (iobjecttype) {
    // level 1, child of database
    case OT_TABLE:
      recType  = OT_STATIC_TABLE; // parentstrings[0] = base name
      break;
    case OT_VIEW:
      recType  = OT_STATIC_VIEW;  // parentstrings[0] = base name
      break;
    case OT_PROCEDURE:
      recType  = OT_STATIC_PROCEDURE;  // parentstrings[0] = base name
      break;
    case OT_SEQUENCE:
      recType  = OT_STATIC_SEQUENCE;   // parentstrings[0] = base name
      break;
    case OT_SCHEMAUSER:
      recType  = OT_STATIC_SCHEMA;  // parentstrings[0] = base name
      break;
    case OT_SYNONYM:
      recType  = OT_STATIC_SYNONYM;  // parentstrings[0] = base name
      break;
    case OT_DBGRANT_ACCESY_USER:
      recType  = OT_STATIC_DBGRANTEES_ACCESY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_ACCESN_USER:
      recType  = OT_STATIC_DBGRANTEES_ACCESN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CREPRY_USER:
      recType  = OT_STATIC_DBGRANTEES_CREPRY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CREPRN_USER:
      recType  = OT_STATIC_DBGRANTEES_CREPRN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CRETBY_USER:
      recType  = OT_STATIC_DBGRANTEES_CRETBY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CRETBN_USER:
      recType  = OT_STATIC_DBGRANTEES_CRETBN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_DBADMY_USER:
      recType  = OT_STATIC_DBGRANTEES_DBADMY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_DBADMN_USER:
      recType  = OT_STATIC_DBGRANTEES_DBADMN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_LKMODY_USER:
      recType  = OT_STATIC_DBGRANTEES_LKMODY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_LKMODN_USER:
      recType  = OT_STATIC_DBGRANTEES_LKMODN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYIOY_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYIOY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYION_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYION; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYRWY_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYRWY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYRWN_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYRWN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_UPDSCY_USER:
      recType  = OT_STATIC_DBGRANTEES_UPDSCY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_UPDSCN_USER:
      recType  = OT_STATIC_DBGRANTEES_UPDSCN; // parentstrings[0] = base
      break;

    case OT_DBGRANT_SELSCY_USER:
      recType  = OT_STATIC_DBGRANTEES_SELSCY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_SELSCN_USER:
      recType  = OT_STATIC_DBGRANTEES_SELSCN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CNCTLY_USER:
      recType  = OT_STATIC_DBGRANTEES_CNCTLY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_CNCTLN_USER:
      recType  = OT_STATIC_DBGRANTEES_CNCTLN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_IDLTLY_USER:
      recType  = OT_STATIC_DBGRANTEES_IDLTLY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_IDLTLN_USER:
      recType  = OT_STATIC_DBGRANTEES_IDLTLN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_SESPRY_USER:
      recType  = OT_STATIC_DBGRANTEES_SESPRY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_SESPRN_USER:
      recType  = OT_STATIC_DBGRANTEES_SESPRN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_TBLSTY_USER:
      recType  = OT_STATIC_DBGRANTEES_TBLSTY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_TBLSTN_USER:
      recType  = OT_STATIC_DBGRANTEES_TBLSTN; // parentstrings[0] = base
      break;

    case OT_DBGRANT_QRYCPY_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYCPY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYCPN_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYCPN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYPGY_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYPGY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYPGN_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYPGN; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYCOY_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYCOY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_QRYCON_USER:
      recType  = OT_STATIC_DBGRANTEES_QRYCON; // parentstrings[0] = base
      break;
    case OT_DBGRANT_SEQCRY_USER:
      recType  = OT_STATIC_DBGRANTEES_CRSEQY; // parentstrings[0] = base
      break;
    case OT_DBGRANT_SEQCRN_USER:
      recType  = OT_STATIC_DBGRANTEES_CRSEQN; // parentstrings[0] = base
      break;

    case OT_DBEVENT:
      recType  = OT_STATIC_DBEVENT;  // parentstrings[0] = base name
      break;

    case OT_S_ALARM_CO_SUCCESS_USER:
      recType = OT_STATIC_DBALARM_CONN_SUCCESS;
      break;
    case OT_S_ALARM_CO_FAILURE_USER:
      recType = OT_STATIC_DBALARM_CONN_FAILURE;
      break;
    case OT_S_ALARM_DI_SUCCESS_USER:
      recType = OT_STATIC_DBALARM_DISCONN_SUCCESS;
      break;
    case OT_S_ALARM_DI_FAILURE_USER:
      recType = OT_STATIC_DBALARM_DISCONN_FAILURE;
      break;

    case OTR_CDB:
      recType  = OT_STATIC_R_CDB; // parentstrings[0] = base name
      break;

    // level 1, child of database/replication
    // parentstrings[0] = base name
    case OT_REPLIC_CONNECTION:
      recType = OT_STATIC_REPLIC_CONNECTION;
      break;
    case OT_REPLIC_CDDS:
      recType = OT_STATIC_REPLIC_CDDS;
      break;
    case OT_REPLIC_MAILUSER:
      recType = OT_STATIC_REPLIC_MAILUSER;
      break;
    case OT_REPLIC_REGTABLE:
      recType = OT_STATIC_REPLIC_REGTABLE;
      break;

    // level 1, child of user
    case OTR_USERSCHEMA:
      recType = OT_STATIC_R_USERSCHEMA; // parentstring[0] = user name
      break;
    case OTR_USERGROUP:
      recType = OT_STATIC_R_USERGROUP; // parentstring[0] = user name
      break;
    case OTR_ALARM_SELSUCCESS_TABLE:
      recType = OT_STATIC_R_SEC_SEL_SUCC;   // parentstring[0] = user name
      break;
    case OTR_ALARM_SELFAILURE_TABLE:
      recType = OT_STATIC_R_SEC_SEL_FAIL;   // parentstring[0] = user name
      break;
    case OTR_ALARM_DELSUCCESS_TABLE:
      recType = OT_STATIC_R_SEC_DEL_SUCC;   // parentstring[0] = user name
      break;
    case OTR_ALARM_DELFAILURE_TABLE:
      recType = OT_STATIC_R_SEC_DEL_FAIL;   // parentstring[0] = user name
      break;
    case OTR_ALARM_INSSUCCESS_TABLE:
      recType = OT_STATIC_R_SEC_INS_SUCC;   // parentstring[0] = user name
      break;
    case OTR_ALARM_INSFAILURE_TABLE:
      recType = OT_STATIC_R_SEC_INS_FAIL;   // parentstring[0] = user name
      break;
    case OTR_ALARM_UPDSUCCESS_TABLE:
      recType = OT_STATIC_R_SEC_UPD_SUCC;   // parentstring[0] = user name
      break;
    case OTR_ALARM_UPDFAILURE_TABLE:
      recType = OT_STATIC_R_SEC_UPD_FAIL;   // parentstring[0] = user name
      break;

    case OTR_GRANTEE_RAISE_DBEVENT:
      recType = OT_STATIC_R_DBEGRANT_RAISE; // parent: user/group/role
      break;
    case OTR_GRANTEE_REGTR_DBEVENT:
      recType = OT_STATIC_R_DBEGRANT_REGISTER;  // parent: user/group/role
      break;
    case OTR_GRANTEE_EXEC_PROC:
      recType = OT_STATIC_R_PROCGRANT_EXEC; // parent: user/group/role
      break;
    case OTR_GRANTEE_NEXT_SEQU:
      recType = OT_STATIC_R_SEQGRANT_NEXT;  // parent: user/group/role
      break;
    case OTR_DBGRANT_ACCESY_DB:
      recType = OT_STATIC_R_DBGRANT_ACCESY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_ACCESN_DB:
      recType = OT_STATIC_R_DBGRANT_ACCESN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CREPRY_DB:
      recType = OT_STATIC_R_DBGRANT_CREPRY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CREPRN_DB:
      recType = OT_STATIC_R_DBGRANT_CREPRN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SEQCRY_DB:
      recType = OT_STATIC_R_DBGRANT_CRESEQY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SEQCRN_DB:
      recType = OT_STATIC_R_DBGRANT_CRESEQN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CRETBY_DB:
      recType = OT_STATIC_R_DBGRANT_CRETBY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CRETBN_DB:
      recType = OT_STATIC_R_DBGRANT_CRETBN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_DBADMY_DB:
      recType = OT_STATIC_R_DBGRANT_DBADMY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_DBADMN_DB:
      recType = OT_STATIC_R_DBGRANT_DBADMN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_LKMODY_DB:
      recType = OT_STATIC_R_DBGRANT_LKMODY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_LKMODN_DB:
      recType = OT_STATIC_R_DBGRANT_LKMODN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYIOY_DB:
      recType = OT_STATIC_R_DBGRANT_QRYIOY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYION_DB:
      recType = OT_STATIC_R_DBGRANT_QRYION;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYRWY_DB:
      recType = OT_STATIC_R_DBGRANT_QRYRWY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYRWN_DB:
      recType = OT_STATIC_R_DBGRANT_QRYRWN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_UPDSCY_DB:
      recType = OT_STATIC_R_DBGRANT_UPDSCY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_UPDSCN_DB:
      recType = OT_STATIC_R_DBGRANT_UPDSCN;  // parent: user/group/role
      break;

    case OTR_DBGRANT_SELSCY_DB:
      recType = OT_STATIC_R_DBGRANT_SELSCY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SELSCN_DB:
      recType = OT_STATIC_R_DBGRANT_SELSCN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CNCTLY_DB:
      recType = OT_STATIC_R_DBGRANT_CNCTLY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_CNCTLN_DB:
      recType = OT_STATIC_R_DBGRANT_CNCTLN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_IDLTLY_DB:
      recType = OT_STATIC_R_DBGRANT_IDLTLY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_IDLTLN_DB:
      recType = OT_STATIC_R_DBGRANT_IDLTLN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SESPRY_DB:
      recType = OT_STATIC_R_DBGRANT_SESPRY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SESPRN_DB:
      recType = OT_STATIC_R_DBGRANT_SESPRN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_TBLSTY_DB:
      recType = OT_STATIC_R_DBGRANT_TBLSTY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_TBLSTN_DB:
      recType = OT_STATIC_R_DBGRANT_TBLSTN;  // parent: user/group/role
      break;

    case OTR_DBGRANT_QRYCPY_DB:
      recType = OT_STATIC_R_DBGRANT_QRYCPY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYCPN_DB:
      recType = OT_STATIC_R_DBGRANT_QRYCPN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYPGY_DB:
      recType = OT_STATIC_R_DBGRANT_QRYPGY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYPGN_DB:
      recType = OT_STATIC_R_DBGRANT_QRYPGN;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYCOY_DB:
      recType = OT_STATIC_R_DBGRANT_QRYCOY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_QRYCON_DB:
      recType = OT_STATIC_R_DBGRANT_QRYCON;  // parent: user/group/role
      break;

    case OTR_GRANTEE_SEL_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_SEL;  // parent: user/group/role
      break;
    case OTR_GRANTEE_INS_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_INS;  // parent: user/group/role
      break;
    case OTR_GRANTEE_UPD_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_UPD;  // parent: user/group/role
      break;
    case OTR_GRANTEE_DEL_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_DEL;  // parent: user/group/role
      break;
    case OTR_GRANTEE_REF_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_REF;  // parent: user/group/role
      break;
    case OTR_GRANTEE_CPI_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_CPI;  // parent: user/group/role
      break;
    case OTR_GRANTEE_CPF_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_CPF;  // parent: user/group/role
      break;
    case OTR_GRANTEE_ALL_TABLE:
      recType = OT_STATIC_R_TABLEGRANT_ALL;  // parent: user/group/role
      break;
    case OTR_GRANTEE_SEL_VIEW:
      recType = OT_STATIC_R_VIEWGRANT_SEL;  // parent: user/group/role
      break;
    case OTR_GRANTEE_INS_VIEW:
      recType = OT_STATIC_R_VIEWGRANT_INS;  // parent: user/group/role
      break;
    case OTR_GRANTEE_UPD_VIEW:
      recType = OT_STATIC_R_VIEWGRANT_UPD;  // parent: user/group/role
      break;
    case OTR_GRANTEE_DEL_VIEW:
      recType = OT_STATIC_R_VIEWGRANT_DEL;  // parent: user/group/role
      break;
    case OTR_GRANTEE_ROLE:
      recType = OT_STATIC_R_ROLEGRANT;      // parent: user
      break;

    // level 1, child of group
    case OT_GROUPUSER:
      recType  = OT_STATIC_GROUPUSER;      // parentstring[0] = group name
      break;

    // level 1, child of role
    case OT_ROLEGRANT_USER:
      recType  = OT_STATIC_ROLEGRANT_USER;  // parentstring[0] = role name
      break;

    // level 1, child of location
    case OTR_LOCATIONTABLE:
      recType = OT_STATIC_R_LOCATIONTABLE;  // parentstring[0]: location name
      break;

    // ICE types , level 1
    case OT_ICE_WEBUSER_ROLE         :
      recType = OT_STATIC_ICE_WEBUSER_ROLE;
      break;
    case OT_ICE_WEBUSER_CONNECTION   :
      recType = OT_STATIC_ICE_WEBUSER_CONNECTION;
      break;
    case OT_ICE_PROFILE_ROLE         :
      recType = OT_STATIC_ICE_PROFILE_ROLE;
      break;
    case OT_ICE_PROFILE_CONNECTION   :
      recType = OT_STATIC_ICE_PROFILE_CONNECTION;
      break;
    case OT_ICE_BUNIT_FACET          :
      recType = OT_STATIC_ICE_BUNIT_FACET;
      break;
    case OT_ICE_BUNIT_PAGE           :
      recType = OT_STATIC_ICE_BUNIT_PAGE;
      break;
    case OT_ICE_BUNIT_LOCATION       :
      recType = OT_STATIC_ICE_BUNIT_LOCATION;
      break;

    default:
      recType = 0;
      break;
  }
  if (recType) {
    // One level of parenthood
    if (previousId)
      if (bTryCurrent)
        id = previousId;
      else
        id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                                                0, (LPARAM)previousId);
    else
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
    while (id) {
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)id);
      if (lpRecord->recType == recType) {
        if (parentstrings[0] != NULL) {
          // search for exact match on parent's name
          if (x_strcmp(lpRecord->extra, parentstrings[0]) == 0)
            return id;
        }
        else {
          // return the parent's name for use in the caller
          lstrcpy(bufPar1, lpRecord->extra);
          return id;
        }
      }

      // manage root item
      if (lpRecord->recType == iobjecttype)
        if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                        0, (LPARAM)id) == 0) {
          *pbRootItem = TRUE;
          return id;
        }

      id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
    }
    return 0;
  }

  // Two levels of parenthood - need to use parentstrings
  switch (iobjecttype) {
    // level 2, child of "table of database"
    case OT_INTEGRITY:
      recType  = OT_STATIC_INTEGRITY; // parentstrings[0] = base name
      break;
    case OT_RULE:
      recType  = OT_STATIC_RULE; // parentstrings[0] = base name
      break;
    case OT_INDEX:
      recType  = OT_STATIC_INDEX; // parentstrings[0] = base name
      break;
    case OT_TABLELOCATION:
      recType  = OT_STATIC_TABLELOCATION; // parentstrings[0] = base name
      break;
    case OT_S_ALARM_SELSUCCESS_USER:
      recType = OT_STATIC_SEC_SEL_SUCC;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_SELFAILURE_USER:
      recType = OT_STATIC_SEC_SEL_FAIL;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_DELSUCCESS_USER:
      recType = OT_STATIC_SEC_DEL_SUCC;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_DELFAILURE_USER:
      recType = OT_STATIC_SEC_DEL_FAIL;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_INSSUCCESS_USER:
      recType = OT_STATIC_SEC_INS_SUCC;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_INSFAILURE_USER:
      recType = OT_STATIC_SEC_INS_FAIL;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_UPDSUCCESS_USER:
      recType = OT_STATIC_SEC_UPD_SUCC;   // parentstring[0] = base name
      break;
    case OT_S_ALARM_UPDFAILURE_USER:
      recType = OT_STATIC_SEC_UPD_FAIL;   // parentstring[0] = base name
      break;

    // grants for tables
    case OT_TABLEGRANT_SEL_USER:
      recType = OT_STATIC_TABLEGRANT_SEL_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_INS_USER:
      recType = OT_STATIC_TABLEGRANT_INS_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_UPD_USER:
      recType = OT_STATIC_TABLEGRANT_UPD_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_DEL_USER:
      recType = OT_STATIC_TABLEGRANT_DEL_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_REF_USER:
      recType = OT_STATIC_TABLEGRANT_REF_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_CPI_USER:
      recType = OT_STATIC_TABLEGRANT_CPI_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_CPF_USER:
      recType = OT_STATIC_TABLEGRANT_CPF_USER;  //parentstring[0] = base name
      break;
    case OT_TABLEGRANT_ALL_USER:
      recType = OT_STATIC_TABLEGRANT_ALL_USER;  //parentstring[0] = base name
      break;
    // desktop
    case OT_TABLEGRANT_INDEX_USER:
      recType = OT_STATIC_TABLEGRANT_INDEX_USER;  //parentstring[0] = base name
      break;
    // desktop
    case OT_TABLEGRANT_ALTER_USER:
      recType = OT_STATIC_TABLEGRANT_ALTER_USER;  //parentstring[0] = base name
      break;

    case OTR_TABLESYNONYM:
      recType = OT_STATIC_R_TABLESYNONYM; // parentstring[0] = base name
      break;
    case OTR_TABLEVIEW:
      recType = OT_STATIC_R_TABLEVIEW;   // parentstring[0] = base name
      break;

    // level 2, child of "view of database"
    case OT_VIEWTABLE:
      recType  = OT_STATIC_VIEWTABLE; // parentstrings[0] = base name
      break;
    case OTR_VIEWSYNONYM:
      recType = OT_STATIC_R_VIEWSYNONYM; // parentstring[0] = base name
      break;

    case OT_VIEWGRANT_SEL_USER:
      recType = OT_STATIC_VIEWGRANT_SEL_USER;  //parentstring[0] = base name
      break;
    case OT_VIEWGRANT_INS_USER:
      recType = OT_STATIC_VIEWGRANT_INS_USER;  //parentstring[0] = base name
      break;
    case OT_VIEWGRANT_UPD_USER:
      recType = OT_STATIC_VIEWGRANT_UPD_USER;  //parentstring[0] = base name
      break;
    case OT_VIEWGRANT_DEL_USER:
      recType = OT_STATIC_VIEWGRANT_DEL_USER;  //parentstring[0] = base name
      break;

    // level 2, child of "procedures of database"
    case OT_PROCGRANT_EXEC_USER:
      recType = OT_STATIC_PROCGRANT_EXEC_USER;
      break;
    case OTR_PROC_RULE:
      recType = OT_STATIC_R_PROC_RULE;
      break;

    // Level 2 child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:
      recType  = OT_STATIC_SEQGRANT_NEXT_USER;   // parentstrings[0] = sequence
      break;

    // level 2, child of "dbevent of database"
    case OT_DBEGRANT_RAISE_USER:
      recType = OT_STATIC_DBEGRANT_RAISE_USER;  //parentstring[0] = base name
      break;
    case OT_DBEGRANT_REGTR_USER:
      recType = OT_STATIC_DBEGRANT_REGTR_USER;  //parentstring[0] = base name
      break;

    // level 2, child of "replication cdds on database"
    // parentstrings[0] = base name
    // parentstrings[1] = cdds name
    case OT_REPLIC_CDDS_DETAIL:
      recType = OT_STATIC_REPLIC_CDDS_DETAIL;
      break;
    case OTR_REPLIC_CDDS_TABLE:
      recType = OT_STATIC_R_REPLIC_CDDS_TABLE;
      break;

    // level 2, child of "table of database"
    // parentstrings[0] = base name
    // parentstrings[1] = table name
    case OTR_REPLIC_TABLE_CDDS:
      recType = OT_STATIC_R_REPLIC_TABLE_CDDS;
      break;

    // level 2, child of "schemauser of database"
    // parentstrings[0] = base name
    // parentstrings[1] = schemauser name
    case OT_SCHEMAUSER_TABLE:
      recType  = OT_STATIC_SCHEMAUSER_TABLE;
      break;
    case OT_SCHEMAUSER_VIEW:
      recType  = OT_STATIC_SCHEMAUSER_VIEW;
      break;
    case OT_SCHEMAUSER_PROCEDURE:
      recType  = OT_STATIC_SCHEMAUSER_PROCEDURE;
      break;

    // ICE types, level 2
    case OT_ICE_BUNIT_SEC_ROLE       :
      recType = OT_STATIC_ICE_BUNIT_SEC_ROLE;
      break;
    case OT_ICE_BUNIT_SEC_USER       :
      recType = OT_STATIC_ICE_BUNIT_SEC_USER;
      break;

    case OT_ICE_BUNIT_FACET_ROLE     :
      recType = OT_STATIC_ICE_BUNIT_FACET_ROLE;
      break;
    case OT_ICE_BUNIT_FACET_USER     :
      recType = OT_STATIC_ICE_BUNIT_FACET_USER;
      break;
    case OT_ICE_BUNIT_PAGE_ROLE     :
      recType = OT_STATIC_ICE_BUNIT_PAGE_ROLE;
      break;
    case OT_ICE_BUNIT_PAGE_USER     :
      recType = OT_STATIC_ICE_BUNIT_PAGE_USER;
      break;

    default:
      recType = 0;
  }
  if (recType) {
    // parentstrings[0] = base name
    // parentstrings[1] = table name/view name/cdds name/class name
    if (previousId)
      if (bTryCurrent)
        id = previousId;
      else
        id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                                                0, (LPARAM)previousId);
    else
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
    while (id) {
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)id);
      if (lpRecord->recType == recType) {
        if (parentstrings[0] != NULL) {      // did we receive a base name ?
          // search for exact match on base name
          if (x_strcmp(lpRecord->extra, parentstrings[0]) == 0) {
            if (parentstrings[1] != NULL) {  // did we receive a table name ?
              // search for exact match on table/view/cdds name
              if (x_strcmp(lpRecord->extra2, parentstrings[1]) == 0)
                return id;
            }
            else {  // didn't receive table/view/cdds/class name,
                    //but received base name
              // return the table name for use in the caller
              lstrcpy(bufPar2, lpRecord->extra2);
              return id;
            }
          }
        }
        else {
          // base name received blank : we ignore the table/view/cdds/class name
          lstrcpy(bufPar2, lpRecord->extra2);   // return table/view/cdds/class name...
          lstrcpy(bufPar1, lpRecord->extra);    // ...AND base name
          return id;
        }
      }


      // manage root item
      if (lpRecord->recType == iobjecttype)
        if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                        0, (LPARAM)id) == 0) {
          *pbRootItem = TRUE;
          return id;
        }

      // No match, let's try next item
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
    }
    return 0;
  }

  // Three levels of parenthood - need to use parentstrings
  switch (iobjecttype) {
    // level 3, child of "index on table of database"
    case OTR_INDEXSYNONYM:
      recType = OT_STATIC_R_INDEXSYNONYM; // parentstring[0] = base name
      // base - table - index
      break;

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:
      recType = OT_STATIC_RULEPROC; // parentstring[0] = base name
      // base - table - rule
      break;
    default:
      recType = 0;
  }
  if (recType) {
    // parentstrings[0] = base name
    // parentstrings[1] = table name
    // parentstrings[2] = index/rule name
    if (previousId)
      if (bTryCurrent)
        id = previousId;
      else
        id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                                                0, (LPARAM)previousId);
    else
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
    while (id) {
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)id);
      if (lpRecord->recType == recType) {
        if (parentstrings[0] != NULL) {      // did we receive a base name ?
          // search for exact match on base name
          if (x_strcmp(lpRecord->extra, parentstrings[0]) == 0) {
            if (parentstrings[1] != NULL) {  // did we receive a table name ?
              // search for exact match on table name
              if (x_strcmp(lpRecord->extra2, parentstrings[1]) == 0)
                if (parentstrings[2] != NULL) {  // did we receive an index/rule name ?
                  // search for exact match on index/rule name
                  if (x_strcmp(lpRecord->extra3, parentstrings[2]) == 0)
                    return id;
                }
                else {  // didn't receive index/rule name, but received base and table name
                  // return the index/rule name for use in the caller
                  lstrcpy(bufPar3, lpRecord->extra3);
                  return id;
                }
            }
            else {  // didn't receive table name, but received base name
              // return the table name and index/rule name for use in the caller
              lstrcpy(bufPar2, lpRecord->extra2);
              lstrcpy(bufPar3, lpRecord->extra3);
              return id;
            }
          }
        }
        else {
          // base name received blank : we ignore the table & index/rule name
          lstrcpy(bufPar3, lpRecord->extra3);   // return index/rule name...
          lstrcpy(bufPar2, lpRecord->extra2);   // ...AND table name...
          lstrcpy(bufPar1, lpRecord->extra);    // ...AND base name
          return id;
        }
      }


      // manage root item
      if (lpRecord->recType == iobjecttype)
        if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                        0, (LPARAM)id) == 0) {
          *pbRootItem = TRUE;
          return id;
        }

      // No match, let's try next item
      id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
    }
    return 0;
  }


  return 0;
}

//
//  Give the object name (OT_xxx) of the tree line received in parameter
//
//  returns NULL if error (empty tree, or record not valid)
//
UCHAR *GetObjName(LPDOMDATA lpDomData, DWORD dwItem)
{
  LPTREERECORD  lpRecord;

  if (dwItem) {
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (lpRecord)
      return lpRecord->objName;
  }
  return NULL;  // Error
}

//
//  Gives the current selection in the tree
//
DWORD GetCurSel(LPDOMDATA lpDomData)
{
  return (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
}

//
//  Give the type (OT_xxx) of the current selection
//
//  returns -1 if error (empty tree, or record not valid)
//
int GetCurSelObjType(LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;

  dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
  if (dwCurSel)
    return GetItemObjType(lpDomData, dwCurSel);
  else
    return -1;  // Error
}

//
//  Give the type (OT_xxx) of the line whose id is received in parameter
//
//  returns -1 if error (lpRecord not valid)
//
int GetItemObjType(LPDOMDATA lpDomData, DWORD dwItem)
{
  LPTREERECORD  lpRecord;

  if (dwItem) {
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (lpRecord)
      return lpRecord->recType;
  }
  return -1;  // Error
}


//
//  Give the display obj type (OT_xxx) of the line whose id is received in parameter
//
//  returns -1 if error (lpRecord not valid)
//
//  duplicated 16/05/97 from GetItemObjType() for fnn request,
//  i.e. show real icon for solved grantes and alarmees
//
int GetItemDisplayObjType(LPDOMDATA lpDomData, DWORD dwItem)
{
  LPTREERECORD  lpRecord;

  if (dwItem) {
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (lpRecord) {
      // Star management: generate several types for star objects according to objects subtypes
      switch (lpRecord->recType) {
        case OT_DATABASE:
          switch (getint(lpRecord->szComplim+STEPSMALLOBJ)) {
            case DBTYPE_REGULAR:
              return OT_DATABASE;
            case DBTYPE_DISTRIBUTED:
              return OT_STAR_DB_DDB;
            case DBTYPE_COORDINATOR:
              return OT_STAR_DB_CDB;
            default:
              return -1;
          }
          break;

        case OT_TABLE:
          switch (getint(lpRecord->szComplim+STEPSMALLOBJ)) {
            case OBJTYPE_NOTSTAR:
              return OT_TABLE;
            case OBJTYPE_STARNATIVE:
              return OT_STAR_TBL_NATIVE;
            case OBJTYPE_STARLINK:
              return OT_STAR_TBL_LINK;
            default:
              return -1;
          }
          break;


        case OT_VIEW:
          switch (getint(lpRecord->szComplim+4+STEPSMALLOBJ)) {
            case OBJTYPE_NOTSTAR:
              return OT_VIEW;
            case OBJTYPE_STARNATIVE:
              return OT_STAR_VIEW_NATIVE;
            case OBJTYPE_STARLINK:
              return OT_STAR_VIEW_LINK;
            default:
              return -1;
          }
          break;

        case OT_PROCEDURE:
          if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
            return OT_STAR_PROCEDURE;
          else
            return OT_PROCEDURE;
          break;

        case OT_INDEX:
          if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
            return OT_STAR_INDEX;
          else
            return OT_INDEX;
          break;

      }
      // end of Star management code

      if (lpRecord->displayType) {
        switch (lpRecord->displayType) {
          case OT_GRANTEE_SOLVED_USER:
          case OT_ALARMEE_SOLVED_USER:
            return OT_USER;
          case OT_GRANTEE_SOLVED_GROUP:
          case OT_ALARMEE_SOLVED_GROUP:
            return OT_GROUP;
          case OT_GRANTEE_SOLVED_ROLE:
          case OT_ALARMEE_SOLVED_ROLE:
            return OT_ROLE;
        }
      }
      return lpRecord->recType;
    }
  }
  return -1;  // Error
}

//
//  Says if the current selection is static or not (OT_STATIC_xxx)
//
//  returns FALSE if error (empty tree, record not valid)
//
BOOL IsCurSelObjStatic(LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;

  dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
  if (dwCurSel)
    return IsItemObjStatic(lpDomData, dwCurSel);
  else
    return FALSE;
}

//
//  Says if the line whose id is received in parameter
//  is static or not (OT_STATIC_xxx)
//
//  returns FALSE if error (empty tree, record not valid)
//
BOOL IsItemObjStatic(LPDOMDATA lpDomData, DWORD dwItem)
{
  int objType;

  objType = GetItemObjType(lpDomData, dwItem);
  switch (objType) {
    // Problem-indicating lines
    case OT_STATIC_ERR_NOGRANT:
    case OT_STATIC_ERR_TIMEOUT:
    case OT_STATIC_ERROR:

    // level 0
    case OT_STATIC_DATABASE:
    case OT_STATIC_PROFILE:
    case OT_STATIC_USER:
    case OT_STATIC_GROUP:
    case OT_STATIC_ROLE:
    case OT_STATIC_LOCATION:
    case OT_STATIC_SYNONYMED:

    // level 1, child of database
    case OT_STATIC_R_CDB:
    case OT_STATIC_TABLE:
    case OT_STATIC_VIEW:
    case OT_STATIC_PROCEDURE:
    case OT_STATIC_SEQUENCE:
    case OT_STATIC_SCHEMA:
    case OT_STATIC_SYNONYM:
    case OT_STATIC_DBGRANTEES:
    case OT_STATIC_DBGRANTEES_ACCESY:
    case OT_STATIC_DBGRANTEES_ACCESN:
    case OT_STATIC_DBGRANTEES_CREPRY:
    case OT_STATIC_DBGRANTEES_CREPRN:
    case OT_STATIC_DBGRANTEES_CRETBY:
    case OT_STATIC_DBGRANTEES_CRETBN:
    case OT_STATIC_DBGRANTEES_DBADMY:
    case OT_STATIC_DBGRANTEES_DBADMN:
    case OT_STATIC_DBGRANTEES_LKMODY:
    case OT_STATIC_DBGRANTEES_LKMODN:
    case OT_STATIC_DBGRANTEES_QRYIOY:
    case OT_STATIC_DBGRANTEES_QRYION:
    case OT_STATIC_DBGRANTEES_QRYRWY:
    case OT_STATIC_DBGRANTEES_QRYRWN:
    case OT_STATIC_DBGRANTEES_UPDSCY:
    case OT_STATIC_DBGRANTEES_UPDSCN:

    case OT_STATIC_DBGRANTEES_SELSCY:
    case OT_STATIC_DBGRANTEES_SELSCN:
    case OT_STATIC_DBGRANTEES_CNCTLY:
    case OT_STATIC_DBGRANTEES_CNCTLN:
    case OT_STATIC_DBGRANTEES_IDLTLY:
    case OT_STATIC_DBGRANTEES_IDLTLN:
    case OT_STATIC_DBGRANTEES_SESPRY:
    case OT_STATIC_DBGRANTEES_SESPRN:
    case OT_STATIC_DBGRANTEES_TBLSTY:
    case OT_STATIC_DBGRANTEES_TBLSTN:

    case OT_STATIC_DBGRANTEES_QRYCPY:
    case OT_STATIC_DBGRANTEES_QRYCPN:
    case OT_STATIC_DBGRANTEES_QRYPGY:
    case OT_STATIC_DBGRANTEES_QRYPGN:
    case OT_STATIC_DBGRANTEES_QRYCOY:
    case OT_STATIC_DBGRANTEES_QRYCON:
    case OT_STATIC_DBGRANTEES_CRSEQY:
    case OT_STATIC_DBGRANTEES_CRSEQN:

    case OT_STATIC_DBEVENT:
    case OT_STATIC_DBALARM:
    case OT_STATIC_DBALARM_CONN_SUCCESS:
    case OT_STATIC_DBALARM_CONN_FAILURE:
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
    case OT_STATIC_DBALARM_DISCONN_FAILURE:

    // level 1, child of user
    case OT_STATIC_R_USERSCHEMA:
    case OT_STATIC_R_USERGROUP:

    case OT_STATIC_R_SECURITY:
    case OT_STATIC_R_SEC_SEL_SUCC:
    case OT_STATIC_R_SEC_SEL_FAIL:
    case OT_STATIC_R_SEC_DEL_SUCC:
    case OT_STATIC_R_SEC_DEL_FAIL:
    case OT_STATIC_R_SEC_INS_SUCC:
    case OT_STATIC_R_SEC_INS_FAIL:
    case OT_STATIC_R_SEC_UPD_SUCC:
    case OT_STATIC_R_SEC_UPD_FAIL:

    case OT_STATIC_R_GRANT:             // for users, groups and roles
    case OT_STATIC_R_DBEGRANT:
    case OT_STATIC_R_DBEGRANT_RAISE:
    case OT_STATIC_R_DBEGRANT_REGISTER:
    case OT_STATIC_R_PROCGRANT:
    case OT_STATIC_R_PROCGRANT_EXEC:
    case OT_STATIC_R_SEQGRANT:
    case OT_STATIC_R_SEQGRANT_NEXT:
    case OT_STATIC_R_DBGRANT:
    case OT_STATIC_R_DBGRANT_ACCESY:
    case OT_STATIC_R_DBGRANT_ACCESN:
    case OT_STATIC_R_DBGRANT_CREPRY:
    case OT_STATIC_R_DBGRANT_CREPRN:
    case OT_STATIC_R_DBGRANT_CRESEQY:
    case OT_STATIC_R_DBGRANT_CRESEQN:
    case OT_STATIC_R_DBGRANT_CRETBY:
    case OT_STATIC_R_DBGRANT_CRETBN:
    case OT_STATIC_R_DBGRANT_DBADMY:
    case OT_STATIC_R_DBGRANT_DBADMN:
    case OT_STATIC_R_DBGRANT_LKMODY:
    case OT_STATIC_R_DBGRANT_LKMODN:
    case OT_STATIC_R_DBGRANT_QRYIOY:
    case OT_STATIC_R_DBGRANT_QRYION:
    case OT_STATIC_R_DBGRANT_QRYRWY:
    case OT_STATIC_R_DBGRANT_QRYRWN:
    case OT_STATIC_R_DBGRANT_UPDSCY:
    case OT_STATIC_R_DBGRANT_UPDSCN:

    case OT_STATIC_R_DBGRANT_SELSCY:
    case OT_STATIC_R_DBGRANT_SELSCN:
    case OT_STATIC_R_DBGRANT_CNCTLY:
    case OT_STATIC_R_DBGRANT_CNCTLN:
    case OT_STATIC_R_DBGRANT_IDLTLY:
    case OT_STATIC_R_DBGRANT_IDLTLN:
    case OT_STATIC_R_DBGRANT_SESPRY:
    case OT_STATIC_R_DBGRANT_SESPRN:
    case OT_STATIC_R_DBGRANT_TBLSTY:
    case OT_STATIC_R_DBGRANT_TBLSTN:

    case OT_STATIC_R_DBGRANT_QRYCPY:
    case OT_STATIC_R_DBGRANT_QRYCPN:
    case OT_STATIC_R_DBGRANT_QRYPGY:
    case OT_STATIC_R_DBGRANT_QRYPGN:
    case OT_STATIC_R_DBGRANT_QRYCOY:
    case OT_STATIC_R_DBGRANT_QRYCON:

    case OT_STATIC_R_TABLEGRANT:
    case OT_STATIC_R_TABLEGRANT_SEL:
    case OT_STATIC_R_TABLEGRANT_INS:
    case OT_STATIC_R_TABLEGRANT_UPD:
    case OT_STATIC_R_TABLEGRANT_DEL:
    case OT_STATIC_R_TABLEGRANT_REF:
    case OT_STATIC_R_TABLEGRANT_CPI:
    case OT_STATIC_R_TABLEGRANT_CPF:
    case OT_STATIC_R_TABLEGRANT_ALL:
    case OT_STATIC_R_VIEWGRANT:
    case OT_STATIC_R_VIEWGRANT_SEL:
    case OT_STATIC_R_VIEWGRANT_INS:
    case OT_STATIC_R_VIEWGRANT_UPD:
    case OT_STATIC_R_VIEWGRANT_DEL:
    case OT_STATIC_R_ROLEGRANT:

    // level 1, child of group
    case OT_STATIC_GROUPUSER:

    // level 1, child of role
    case OT_STATIC_ROLEGRANT_USER:

    // level 1, child of location
    case OT_STATIC_R_LOCATIONTABLE:

    // level 2, child of "table of database"
    case OT_STATIC_INTEGRITY:
    case OT_STATIC_RULE:
    case OT_STATIC_INDEX:
    case OT_STATIC_TABLELOCATION:

    case OT_STATIC_SECURITY:
    case OT_STATIC_SEC_SEL_SUCC:
    case OT_STATIC_SEC_SEL_FAIL:
    case OT_STATIC_SEC_DEL_SUCC:
    case OT_STATIC_SEC_DEL_FAIL:
    case OT_STATIC_SEC_INS_SUCC:
    case OT_STATIC_SEC_INS_FAIL:
    case OT_STATIC_SEC_UPD_SUCC:
    case OT_STATIC_SEC_UPD_FAIL:

    case OT_STATIC_R_TABLESYNONYM:
    case OT_STATIC_R_TABLEVIEW:

    case OT_STATIC_TABLEGRANTEES:
    case OT_STATIC_TABLEGRANT_SEL_USER:
    case OT_STATIC_TABLEGRANT_INS_USER:
    case OT_STATIC_TABLEGRANT_UPD_USER:
    case OT_STATIC_TABLEGRANT_DEL_USER:
    case OT_STATIC_TABLEGRANT_REF_USER:
    case OT_STATIC_TABLEGRANT_CPI_USER:
    case OT_STATIC_TABLEGRANT_CPF_USER:
    case OT_STATIC_TABLEGRANT_ALL_USER:
    case OT_STATIC_TABLEGRANT_INDEX_USER:
    case OT_STATIC_TABLEGRANT_ALTER_USER:

    // level 2, child of "view of database"
    case OT_STATIC_VIEWTABLE:
    case OT_STATIC_R_VIEWSYNONYM:

    case OT_STATIC_VIEWGRANTEES:
    case OT_STATIC_VIEWGRANT_SEL_USER:
    case OT_STATIC_VIEWGRANT_INS_USER:
    case OT_STATIC_VIEWGRANT_UPD_USER:
    case OT_STATIC_VIEWGRANT_DEL_USER:

    // level 2, child of "procedure of database"
    case OT_STATIC_PROCGRANT_EXEC_USER:
    case OT_STATIC_R_PROC_RULE:

    // level 2, child of "dbevent of database"
    case OT_STATIC_DBEGRANT_RAISE_USER:
    case OT_STATIC_DBEGRANT_REGTR_USER:

    // level 2, child of "sequence of database"
    case OT_STATIC_SEQGRANT_NEXT_USER:

    // level 2, child of "schemauser on database"
    case OT_STATIC_SCHEMAUSER_TABLE:
    case OT_STATIC_SCHEMAUSER_VIEW:
    case OT_STATIC_SCHEMAUSER_PROCEDURE:

    // level 3, child of "index on table of database"
    case OT_STATIC_R_INDEXSYNONYM:

    // level 3, child of "rule on table of database"
    case OT_STATIC_RULEPROC:

    // replicator, all levels mixed
    case OT_STATIC_REPLICATOR:

    case OT_STATIC_REPLIC_CONNECTION:
    case OT_STATIC_REPLIC_CDDS:
    case OT_STATIC_REPLIC_MAILUSER:
    case OT_STATIC_REPLIC_REGTABLE:

    case OT_STATIC_REPLIC_CDDS_DETAIL:
    case OT_STATIC_R_REPLIC_CDDS_TABLE:

    case OT_STATIC_R_REPLIC_TABLE_CDDS:

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_STATIC_ALARMEE:
    case OT_STATIC_ALARM_EVENT:

    //
    // ICE
    //
    case OT_STATIC_ICE                      :
    // Under "Security"
    case OT_STATIC_ICE_SECURITY             :
    case OT_STATIC_ICE_ROLE               :
    case OT_STATIC_ICE_DBUSER               :
    case OT_STATIC_ICE_DBCONNECTION         :
    case OT_STATIC_ICE_WEBUSER              :
    case OT_STATIC_ICE_WEBUSER_ROLE         :
    case OT_STATIC_ICE_WEBUSER_CONNECTION   :
    case OT_STATIC_ICE_PROFILE              :
    case OT_STATIC_ICE_PROFILE_ROLE         :
    case OT_STATIC_ICE_PROFILE_CONNECTION   :
    // Under "Bussiness unit" (BUNIT)
    case OT_STATIC_ICE_BUNIT                :
    case OT_STATIC_ICE_BUNIT_SECURITY       :
    case OT_STATIC_ICE_BUNIT_SEC_ROLE       :
    case OT_STATIC_ICE_BUNIT_SEC_USER       :
    case OT_STATIC_ICE_BUNIT_FACET          :
    case OT_STATIC_ICE_BUNIT_PAGE           :
    case OT_STATIC_ICE_BUNIT_FACET_ROLE     :
    case OT_STATIC_ICE_BUNIT_FACET_USER     :
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE      :
    case OT_STATIC_ICE_BUNIT_PAGE_USER      :
    case OT_STATIC_ICE_BUNIT_LOCATION       :
    // Under "Server"
    case OT_STATIC_ICE_SERVER               :
    case OT_STATIC_ICE_SERVER_APPLICATION   :
    case OT_STATIC_ICE_SERVER_LOCATION      :
    case OT_STATIC_ICE_SERVER_VARIABLE      :

    //
    // INSTALLATION LEVEL SETTINGS
    //
    case OT_STATIC_INSTALL                  :
    //case OT_STATIC_INSTALL_SECURITY         :
    case OT_STATIC_INSTALL_GRANTEES         :
    case OT_STATIC_INSTALL_ALARMS           :

      return TRUE;

    default:
      // non-static, or empty tree
      return FALSE;
  }

  // Security
  return FALSE;   // Error
}

//
//  returns the previous sibling line, or 0L if none
//
DWORD GetPreviousSibling(LPDOMDATA lpDomData, DWORD dwReq)
{
#ifdef WIN95_CONTROLS
  return SendMessage(lpDomData->hwndTreeLb, LM_GETPREVSIBLING,
                     0, (LPARAM)dwReq);
#else   // WIN95_CONTROLS

  DWORD idSibling;
  DWORD reqLevel;
  DWORD curLevel;

  reqLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                            0, (LPARAM)dwReq);
  idSibling = SendMessage(lpDomData->hwndTreeLb, LM_GETPREV,
                        0, (LPARAM)dwReq);
  while (idSibling) {
    curLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                                                  0, (LPARAM)idSibling);
    if (curLevel < reqLevel)
      return 0L;

    if (curLevel == reqLevel)
      return idSibling;   // Successful!
    idSibling = SendMessage(lpDomData->hwndTreeLb, LM_GETPREV, 0, idSibling);
  }
  return 0L;    // no more lines
#endif   // WIN95_CONTROLS
}

//
//  returns the next sibling line, or 0L if none
//
DWORD GetNextSibling(LPDOMDATA lpDomData, DWORD dwReq)
{
#ifdef WIN95_CONTROLS
  return SendMessage(lpDomData->hwndTreeLb, LM_GETNEXTSIBLING,
                     0, (LPARAM)dwReq);
#else   // WIN95_CONTROLS

  DWORD idSibling;
  DWORD reqLevel;
  DWORD curLevel;

  reqLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                            0, (LPARAM)dwReq);
  idSibling = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)dwReq);
  while (idSibling) {
    curLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                                                  0, (LPARAM)idSibling);
    if (curLevel < reqLevel)
      return 0L;

    if (curLevel == reqLevel)
      return idSibling;   // Successful!
    idSibling = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, idSibling);
  }
  return 0L;    // no more lines
#endif   // WIN95_CONTROLS
}

//
//  returns the first immediate child of the idParentReq tree line
//
//  returns 0L if no child
//
//  IMPORTANT : must be called prior to calling GetNextImmediateChild
//
//  28/6/95 : Tree linked list optimization purposes:
//  returns the level of the parent, to be passed to GetNextImmediateChild
//  if the received pointer (pParentLevel) is not null
//
DWORD GetFirstImmediateChild(LPDOMDATA lpDomData, DWORD idParentReq, DWORD *pParentLevel)
{
  if (pParentLevel)
#ifdef WIN95_CONTROLS
    *pParentLevel = 0;  // Useless and Time-consumer in Windows 95
#else   // WIN95_CONTROLS
    *pParentLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                              0, (LPARAM)idParentReq);
#endif  // WIN95_CONTROLS
  return (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETFIRSTCHILD,
                            0, (LPARAM)idParentReq);
}

//
//  returns the next immediate child of the idParentReq tree line
//
//  returns 0L if no more child
//
//  IMPORTANT : GetFirstImmediateChild or GetNextImmediateChild
// must have been called prior to calling this function
//
//  28/6/95 : Tree linked list optimization purposes:
//  uses the received level of the parent (found with GetFirstImmediateChild)
//  except if -1L, indicating the function has to find
// the parent level herself
//
DWORD GetNextImmediateChild(LPDOMDATA lpDomData, DWORD idParentReq, DWORD idCurChild, DWORD ParentLevel)
{
#ifdef WIN95_CONTROLS
  // idParentReq and ParentLevel are useless
  return SendMessage(lpDomData->hwndTreeLb, LM_GETNEXTSIBLING,
                     0, (LPARAM)idCurChild);
#else   // WIN95_CONTROLS

  DWORD idChild;
  DWORD idParent;
  DWORD ChildLevel;

  if (ParentLevel == -1L)
    ParentLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                              0, (LPARAM)idParentReq);
  idChild = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)idCurChild);
  while (idChild) {
    ChildLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                              0, (LPARAM)idChild);
    if (ChildLevel <= ParentLevel)
      return 0L;          // next branch reached
    if (ChildLevel == ParentLevel + 1) {
      // Second check on parent id
      idParent = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                    0, (LPARAM)idChild);
      if (idParent==idParentReq)
        return idChild;   // Successful!
      else
        return 0L;        // Erroneous situation !
    }
    idChild = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, 0L);
  }
  return 0L;    // no more lines
#endif  // WIN95_CONTROLS
}

//
//  returns the first level 0 tree line
//
//  returns 0L if no child
//
//  IMPORTANT : must be called prior to calling GetNextLevel0Item
//
DWORD GetFirstLevel0Item(LPDOMDATA lpDomData)
{
  return (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
}

//
//  returns the next level 0 tree line
//
//  returns 0L if no more level 0 line
//
//  IMPORTANT : GetFirstLevel0Item or GetNextLevel0Item
// must have been called prior to calling this function
//
DWORD GetNextLevel0Item(LPDOMDATA lpDomData, DWORD idCurItem)
{
  DWORD idChild;
  DWORD ChildLevel;

  idChild = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)idCurItem);
  while (idChild) {
    ChildLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                              0, (LPARAM)idChild);
    if (ChildLevel == 0)
      return idChild;   // Successful!
    idChild = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, 0L);
  }
  return 0L;    // no more lines
}

//
//  returns the id of the immediate child of the idParentReq tree line
//  whose name is identical to objname
//
//  Optimized 10/03/95 : stops on a given child (last of the existing
//    children before we began to add items)
//
//  Modified 15/3/95: also receives the item type, to call the right
//  compare function, plus first parent name and complimentary, that
//  might be used in the compare functions
//
//  returns 0L if no matching immediate child
//
DWORD FindImmediateSubItem(LPDOMDATA lpDomData, 
                           DWORD idParentReq, 
                           DWORD idFirstChildObj, 
                           DWORD idLastChildObj, 
                           int iobjecttype, 
                           LPUCHAR objName, 
                           LPUCHAR objOwnerName,
                           LPUCHAR parentName, 
                           LPUCHAR bufComplim, 
                           DWORD *pidInsertAfter, 
                           DWORD *pidInsertBefore)
{
  DWORD         idCurChild;   // current child
  DWORD         idPrevChild;  // previous child, for sort management
  DWORD         dwParent;     // Parent id.
  LPUCHAR       curName;
  LPUCHAR       curParent0;
  long          curComplimValue;
  long          complimValue;
  LPTREERECORD  lpRecord;
  int           val;            // compare result value
  DWORD         dwParentLevel;

  //#define NO_USE_OF_IDFIRST
  #ifdef NO_USE_OF_IDFIRST
  idFirstChildObj = 0L;
  #endif // NO_USE_OF_IDFIRST

  *pidInsertAfter   = 0L;   // default value ---> insert at the end of list
  *pidInsertBefore  = 0L;   // default value ---> insertAfter will be used

  idCurChild = GetFirstImmediateChild(lpDomData, idParentReq, &dwParentLevel);
  idPrevChild = idParentReq;
  if (idFirstChildObj) {
   dwParent = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                                       0, (LPARAM)idFirstChildObj);  

   // Do not loop unnecessarily if id of the last child object is not set 
   // and if the Parent id of first child is same as that of idParentReq. 
   if ((idLastChildObj!=0) && (dwParent == idParentReq))
   {
     idCurChild = idFirstChildObj;
   }
   else
   {
    while (1) {
      // Don't go too far... We would loop uselesless if the previously
      // inserted child was inserted AFTER the LastChildObj. This typically
      // happends when we expand a branch for the first time, for each item.
      if (idCurChild == idLastChildObj)
        break;

      // Stop on the right one!
      if (idCurChild == idFirstChildObj)
        break;

      if (idCurChild ==0) {
        // ERROR! WE REACHED THE END OF THE IMMEDIATE CHILDREN WITHOUT
        // FINDING THE STARTING IMMEDIATE CHILD!
        // We say to insert after the parent
        *pidInsertAfter = idParentReq;
        return 0L;
      }

      // next immediate child, please...
      idPrevChild = idCurChild;
      idCurChild = GetNextImmediateChild(lpDomData, idParentReq, idCurChild,
                                         dwParentLevel);
    }
   } 
  }
  while (idCurChild) {
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)idCurChild);
    if (!lpRecord)
      return 0L;    // ERROR : exit immediately, will insert at end of list

    // Skip special item for tree image bug workaround, see domdisp.c
    if (lpRecord->recType == -1) {
	     // next child
    	idPrevChild = idCurChild;
    	idCurChild = GetNextImmediateChild(lpDomData, idParentReq, idCurChild,
        	                               dwParentLevel);
       continue;
    }

    switch (iobjecttype) {
      case OTR_LOCATIONTABLE:
      case OTR_GRANTEE_SEL_TABLE:
      case OTR_GRANTEE_INS_TABLE:
      case OTR_GRANTEE_UPD_TABLE:
      case OTR_GRANTEE_DEL_TABLE:
      case OTR_GRANTEE_REF_TABLE:
      case OTR_GRANTEE_CPI_TABLE:
      case OTR_GRANTEE_CPF_TABLE:
      case OTR_GRANTEE_ALL_TABLE:

      case OTR_TABLEVIEW:
      case OTR_GRANTEE_SEL_VIEW:
      case OTR_GRANTEE_INS_VIEW:
      case OTR_GRANTEE_UPD_VIEW:
      case OTR_GRANTEE_DEL_VIEW:

      case OTR_GRANTEE_RAISE_DBEVENT:
      case OTR_GRANTEE_REGTR_DBEVENT:

      case OTR_GRANTEE_EXEC_PROC:
      case OTR_GRANTEE_NEXT_SEQU:

        curName = lpRecord->objName;
        curParent0 = lpRecord->extra;
        val = DOMTreeCmpTablesWithDB(parentName, objName, objOwnerName,
                                     curParent0, curName, lpRecord->ownerName);
        break;

      case OT_S_ALARM_SELSUCCESS_USER:    // table alarms
      case OT_S_ALARM_SELFAILURE_USER:
      case OT_S_ALARM_DELSUCCESS_USER:
      case OT_S_ALARM_DELFAILURE_USER:
      case OT_S_ALARM_INSSUCCESS_USER:
      case OT_S_ALARM_INSFAILURE_USER:
      case OT_S_ALARM_UPDSUCCESS_USER:
      case OT_S_ALARM_UPDFAILURE_USER:
      case OT_S_ALARM_CO_SUCCESS_USER:    // database alarms
      case OT_S_ALARM_CO_FAILURE_USER:
      case OT_S_ALARM_DI_SUCCESS_USER:
      case OT_S_ALARM_DI_FAILURE_USER:
      case OT_INTEGRITY :                 // integrities
        curName = lpRecord->objName;
        curComplimValue = lpRecord->complimValue;
        complimValue = (long)getint(bufComplim);
        val = DOMTreeCmpSecAlarms(objName, complimValue,
                                curName, curComplimValue);
        break;

      case OTR_ALARM_SELSUCCESS_TABLE:
      case OTR_ALARM_SELFAILURE_TABLE:
      case OTR_ALARM_DELSUCCESS_TABLE:
      case OTR_ALARM_DELFAILURE_TABLE:
      case OTR_ALARM_INSSUCCESS_TABLE:
      case OTR_ALARM_INSFAILURE_TABLE:
      case OTR_ALARM_UPDSUCCESS_TABLE:
      case OTR_ALARM_UPDFAILURE_TABLE:
        curName = lpRecord->objName;
        curParent0 = lpRecord->extra;
        curComplimValue = lpRecord->complimValue;
        complimValue = (long)getint(bufComplim);
        // compare first parent name, then object name, then complimValue
        val = DOMTreeCmpRelSecAlarms(parentName, objName, objOwnerName, complimValue,
                                     curParent0, curName, lpRecord->ownerName, curComplimValue);
        break;

      case OT_REPLIC_CONNECTION:
      case OT_REPLIC_CDDS:
        // numeric compare only : not enough since empty buffer equal to 0
        curComplimValue = lpRecord->complimValue;
        complimValue = (long)getint(objOwnerName);
        if (complimValue == curComplimValue)
          if (lpRecord->objName[0] != '\0')
            val = 0;
          else
            val = 1;
        else
          if (complimValue > curComplimValue)
            val = 1;
          else
            val = -1;
        break;

      case OT_REPLIC_CDDS_DETAIL:
        val = DOMTreeCmp4ints(
                      getint(bufComplim),
                      getint(bufComplim + STEPSMALLOBJ),
                      getint(bufComplim + 2 * STEPSMALLOBJ),
                      0,
                      getint(lpRecord->szComplim),
                      getint(lpRecord->szComplim + STEPSMALLOBJ),
                      getint(lpRecord->szComplim + 2 * STEPSMALLOBJ),
                      0);
        break;

      default:
        curName = lpRecord->objName;
        val = DOMTreeCmpStr(objName, objOwnerName,
                            curName, lpRecord->ownerName, iobjecttype, FALSE);
        if (val==0)
         if (x_strcmp(objName, curName)) {
            SETSTRING setString;    // for tree line caption update

            x_strcpy(lpRecord->objName, objName);
            setString.lpString  = objName;
            setString.ulRecID   = idCurChild;
            SendMessage(lpDomData->hwndTreeLb, LM_SETSTRING, 0,
                        (LPARAM) (LPSETSTRING)&setString );
         }
        break;
    }

    // test compare return value
    if (val==0)
      return idCurChild;  // keep *pidInsertAfter set to null
    if (val < 0) {
      // our object is "of lower value" than the current one:
      // since existing objects are assumed to be already sorted,
      // we will insert just after the previous item, or just before
      // the current item if it is the first child
      if (idPrevChild==idParentReq)
        *pidInsertBefore = idCurChild;
      else
        *pidInsertAfter = idPrevChild;
      return 0L;    // no need to continue the loop since sort assumed
    }

    // test whether we reached the last child that already existed
    // if it is the case, insert at the end
    if (idCurChild == idLastChildObj) {
      *pidInsertAfter = 0L;
      return 0L;
    }

    // next child
    idPrevChild = idCurChild;
    idCurChild = GetNextImmediateChild(lpDomData, idParentReq, idCurChild,
                                       dwParentLevel);
  }

  return 0L;
}

//
//  deletes all children of a branch with their sub-children
//
//  returns void
//
VOID TreeDeleteAllChildren(LPDOMDATA lpDomData, DWORD idParent)
{
  DWORD idChildObj;

  while (1) {
    idChildObj = GetFirstImmediateChild(lpDomData, idParent, 0);
    if (idChildObj==0)
      break;      // No more children

    // recursively delete children of this child
    TreeDeleteAllChildren(lpDomData, idChildObj);

    // delete the child after all children have been deleted
    TreeDeleteRecord(lpDomData, idChildObj);
  }
}

//
//  deletes all children of a branch marked for deletion,
//  with their sub-children
//
//  Optimized 10/03/95 : stops on a given child (last of the existing
//    children before we began to add items)
//
//  returns void
//
extern VOID TreeDeleteAllMarkedChildren(LPDOMDATA lpDomData, DWORD idParent, DWORD idLastChildObj)
{
  DWORD         idChild;    // immediate child
  DWORD         idChild2;   // buffered child id
  LPTREERECORD  lpRecord;
  DWORD         dwParentLevel;

  idChild = GetFirstImmediateChild(lpDomData, idParent, &dwParentLevel);
  while (idChild) {
    // get next child in buffered id BEFORE possibly deleting current child
    idChild2 = GetNextImmediateChild(lpDomData, idParent, idChild, dwParentLevel);

    // get info on delete state and delete if necessary
    lpRecord = (LPTREERECORD)SendMessage(lpDomData->hwndTreeLb,
                                         LM_GETITEMDATA,
                                         0, (LPARAM)idChild);
    if (lpRecord && lpRecord->bToBeDeleted) {
      TreeDeleteAllChildren(lpDomData, idChild);  // del. children of branch
      TreeDeleteRecord(lpDomData, idChild);       // delete branch
    }

    // stop if we reached the last child that might be marked for deletion
    if (idChild == idLastChildObj)
      break;

    // put buffered id in current id and loop again
    idChild = idChild2;

  }
}

//
//  On-the-fly type solve, ONLY for TearOut/RestartFromPos
//  Do not use for type solve at expand time, type would be wrong!
//  Also used for Clipboard
//
//  Returns the solved type after having updated the received structure
//  according to the DOMGetObject() output data if bUpdateRec was set
//
int SolveType(LPDOMDATA lpDomData, LPTREERECORD lpRecord, BOOL bUpdateRec)
{
  int           objType = -1;
  int           level;
  LPUCHAR       aparents[MAXPLEVEL];

  // result variables
  int           iret;
  LPUCHAR       aparentsResult[MAXPLEVEL];      // result aparents
  UCHAR         bufParResult0[MAXOBJECTNAME];   // result parent level 0
  UCHAR         bufParResult1[MAXOBJECTNAME];   // result parent level 1
  UCHAR         bufParResult2[MAXOBJECTNAME];   // result parent level 2
  int           resultType;
  int           resultLevel;
  UCHAR         buf[BUFSIZE];
  UCHAR         bufOwner[MAXOBJECTNAME];
  UCHAR         bufComplim[MAXOBJECTNAME];
  BOOL          bStoreUnsolved = FALSE;

  // Pre-test empty record
  if (!lpRecord)
    return 0;

  // filter switch
  switch (lpRecord->recType) {
    case OT_DBGRANT_ACCESY_USER:
    case OT_DBGRANT_ACCESN_USER:
    case OT_DBGRANT_CREPRY_USER:
    case OT_DBGRANT_CREPRN_USER:
    case OT_DBGRANT_CRETBY_USER:
    case OT_DBGRANT_CRETBN_USER:
    case OT_DBGRANT_DBADMY_USER:
    case OT_DBGRANT_DBADMN_USER:
    case OT_DBGRANT_LKMODY_USER:
    case OT_DBGRANT_LKMODN_USER:
    case OT_DBGRANT_QRYIOY_USER:
    case OT_DBGRANT_QRYION_USER:
    case OT_DBGRANT_QRYRWY_USER:
    case OT_DBGRANT_QRYRWN_USER:
    case OT_DBGRANT_UPDSCY_USER:
    case OT_DBGRANT_UPDSCN_USER:

    case OT_DBGRANT_SELSCY_USER:
    case OT_DBGRANT_SELSCN_USER:
    case OT_DBGRANT_CNCTLY_USER:
    case OT_DBGRANT_CNCTLN_USER:
    case OT_DBGRANT_IDLTLY_USER:
    case OT_DBGRANT_IDLTLN_USER:
    case OT_DBGRANT_SESPRY_USER:
    case OT_DBGRANT_SESPRN_USER:
    case OT_DBGRANT_TBLSTY_USER:
    case OT_DBGRANT_TBLSTN_USER:

    case OT_DBGRANT_QRYCPY_USER:
    case OT_DBGRANT_QRYCPN_USER:
    case OT_DBGRANT_QRYPGY_USER:
    case OT_DBGRANT_QRYPGN_USER:
    case OT_DBGRANT_QRYCOY_USER:
    case OT_DBGRANT_QRYCON_USER:
    case OT_DBGRANT_SEQCRY_USER:
    case OT_DBGRANT_SEQCRN_USER:

    case OT_TABLEGRANT_SEL_USER:
    case OT_TABLEGRANT_INS_USER:
    case OT_TABLEGRANT_UPD_USER:
    case OT_TABLEGRANT_DEL_USER:
    case OT_TABLEGRANT_REF_USER:
    case OT_TABLEGRANT_CPI_USER:
    case OT_TABLEGRANT_CPF_USER:
    case OT_TABLEGRANT_ALL_USER:
    case OT_TABLEGRANT_INDEX_USER:  // desktop
    case OT_TABLEGRANT_ALTER_USER:  // desktop

    case OT_VIEWGRANT_SEL_USER:
    case OT_VIEWGRANT_INS_USER:
    case OT_VIEWGRANT_UPD_USER:
    case OT_VIEWGRANT_DEL_USER:

    case OT_DBEGRANT_RAISE_USER:
    case OT_DBEGRANT_REGTR_USER:

    case OT_PROCGRANT_EXEC_USER:
    case OT_SEQUGRANT_NEXT_USER:

      bStoreUnsolved = TRUE;
      objType = OT_GRANTEE;
      level   = 0;
      break;


    // Alarmees for tables and for databases
    case OT_ALARMEE:

      bStoreUnsolved = TRUE;
      objType = OT_GRANTEE;
      level   = 0;
      break;

    case OT_VIEWTABLE:
      bStoreUnsolved = TRUE;
      objType = lpRecord->recType;
      level   = 1;
      break;

    case OT_SYNONYMOBJECT:
      objType = lpRecord->recType;
      level   = 1;
      break;

    default:
      return lpRecord->recType;
  }

  // prepare aparents
  aparents[0] = lpRecord->extra;
  aparents[1] = lpRecord->extra2;
  aparents[2] = lpRecord->extra3;

  // clean result buffers
  bufParResult0[0] = bufParResult1[0] = bufParResult2[0] = '\0';
  bufOwner[0] = bufComplim[0] = '\0';

  // anchor aparentsResult on result buffers
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                      lpRecord->objName,
                      lpRecord->ownerName,
                      objType,
                      level,
                      aparents,
                      TRUE,             // bwithsystem
                      &resultType,
                      &resultLevel,
                      aparentsResult, // array of result parent strings
                      buf,            // result object name
                      bufOwner,       // result owner
                      bufComplim);    // result complimentary data

  if (iret == RES_ENDOFDATA)
    return lpRecord->recType;   // object has previously been deleted!

  // Update the object
  if (bUpdateRec) {
    if (bStoreUnsolved) {
      if (lpRecord->unsolvedRecType == -1)
        lpRecord->unsolvedRecType = lpRecord->recType;
    }
    lpRecord->recType = resultType;
    /* Masqued not to loose parenthood (ex: grantee on what?
    lstrcpy(lpRecord->extra,      aparentsResult[0]);
    lstrcpy(lpRecord->extra2,     aparentsResult[1]);
    */
    lstrcpy(lpRecord->extra3,     aparentsResult[2]);
    lstrcpy(lpRecord->objName,    buf);
    lstrcpy(lpRecord->ownerName,  bufOwner);
    // Emb Feb. 20, 98: memcpy needed for star flags that would otherwise be lost - replaces lstrcpy
    memcpy(lpRecord->szComplim,  bufComplim, sizeof(bufComplim));
  }

  return resultType;
}

//
// returns the number of objects of same type and parenthood
// than the currently selected item in the tree
// returns -1L if number cannot be evaluated
// returns the previous number if the selection has not changed,
// except if forced with bForce set to TRUE
//
//  Development issue: MUST update lastNumber before returning any value.
//
long GetNumberOfObjects(LPDOMDATA lpDomData, BOOL bForce)
{
  long          cpt;
  DWORD         dwCurSel;
  DWORD         dwParent, dwChild;
  LPTREERECORD  lpRecord;
  DWORD         dwParentLevel;
  int           objType;

  // calls optimization
  static        LPDOMDATA lastLpDomData;    // can be static
  static        DWORD     lastCurSel;       // can be static
  static        long      lastNumber;       // can be static

  dwCurSel = GetCurSel(lpDomData);
  if (dwCurSel == 0)
    return -1L;       // empty document (scratchpad)

  // optimized calls to lbtree dll
  if (lpDomData == lastLpDomData && dwCurSel == lastCurSel)
    if (!bForce)
      return lastNumber;

  lastCurSel = dwCurSel;
  lastLpDomData = lpDomData;

  if (IsCurSelObjStatic(lpDomData)) {
    objType = GetItemObjType(lpDomData, dwCurSel);

    // eliminate cases when we would only have sub-statics
    switch (objType) {
      case OT_STATIC_DBGRANTEES:
      case OT_STATIC_REPLICATOR:
      case OT_STATIC_R_GRANT:
      case OT_STATIC_DBALARM:
      case OT_STATIC_R_SECURITY:
      case OT_STATIC_R_DBGRANT:
      case OT_STATIC_R_TABLEGRANT:
      case OT_STATIC_R_VIEWGRANT:
      case OT_STATIC_R_DBEGRANT:
      case OT_STATIC_R_PROCGRANT:
      case OT_STATIC_R_SEQGRANT:
      case OT_STATIC_SECURITY:
      case OT_STATIC_TABLEGRANTEES:
      case OT_STATIC_VIEWGRANTEES:
        lastNumber = -1L;   // for next optimized call
        return -1L;         // only sub-statics for these items
    }

    dwParent = dwCurSel;
    cpt = 0L;
    dwChild = GetFirstImmediateChild(lpDomData, dwParent, &dwParentLevel);
    if (!dwChild) {
      lastNumber = -1L;
      return -1L;
    }
    // Fix Nov 4, 97: must distinguish between "not expanded yet" and "<No item>"
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwParent);
    if (!lpRecord->bSubValid) {
      lastNumber = -1L;
      return -1L;    // branch not expanded yet
    }
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwChild);
    if (lpRecord->objName[0]=='\0') {
      lastNumber = 0L;
      return 0L;    // empty item: branch not expanded yet
    }
    while (dwChild) {
      cpt++;
      dwChild = GetNextImmediateChild(lpDomData, dwParent, dwChild, dwParentLevel);
    }
    lastNumber = cpt;
    return cpt;
  }

  // code for non-static object here

  dwParent = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                                       0, (LPARAM)dwCurSel);
  if (!dwParent) {
    lastNumber = -1L;
    return -1L;
  }

  cpt = 0L;
  dwChild = GetFirstImmediateChild(lpDomData, dwParent, &dwParentLevel);
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwChild);
  if (lpRecord->objName[0]=='\0') {
    lastNumber = 0L;
    return 0L;    // item with no name: <No xxx> ---> 0 item
  }
  while (dwChild) {
    cpt++;
    dwChild = GetNextImmediateChild(lpDomData, dwParent, dwChild, dwParentLevel);
  }
  lastNumber = cpt;
  return cpt;
}

//
//  Get the id of the last record in the tree
//
DWORD GetLastRecordId(LPDOMDATA lpDomData)
{
  DWORD curRecordId;
  DWORD prevRecordId;

  prevRecordId = 0L;
  curRecordId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
  while (curRecordId) {
    prevRecordId = curRecordId;
    curRecordId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, 0L);
  }
  return prevRecordId;
}
