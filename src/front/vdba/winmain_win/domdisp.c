#ifdef WIN32
#define WIN95_CONTROLS
#endif
/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Ingres Visual DBA
**
**  Source : domdisp.c
**  Manages the dom trees display update according to fresh information
**  supplied by calling DomGetFirstObject/DomGetNextObject
**  Tree branches states (expanded/collapsed) remain intact
**
**  Author : Emmanuel Blattes
**
**  History after 01-01-2000
**
**  20-01-2000 (noifr01)
**   (bug 100063) (remove build warnings). At 3 places, the TreeAddRecord()
**   function was called with uninitialized "before" and "after" record IDs.
**   Since in these particular sub-branches, there is only one item,
**   the sort does not matter, which allows to pass 0 for the values (these
**   are convenient values when no sort applies)
**  26-May-2000 (noifr01)
**   bug #99242  cleanup for DBCS compliance
**  06-Mars-2001 (schph01)
**   sir 97068 If the View was created with QUEL language display
**   only the branch "view components".
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**   (sir 104378) differentiation of II 2.6.
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows:
**   force refresh now refreshes all data
**  18-Mar-2003 (schph01 and noifr01)
**   (sir 107523) manage sequences
**  16-Apr-2003 (noifr01)
**   (bug 109548) don't append any more the version of ingres in the
**   caption of "Replication" branches in DOM windows, for versions 
**   >= 2.5
**  23-Jan-2004 (schph01)
**   (sir 104378) detect version 3 of Ingres, for managing
**   new features provided in this version. replaced references
**   to 2.65 with refereces to 3  in #define definitions for
**   better readability in the future
**  06-Feb-2004 (schph01)
**    SIR 111752 Add new branch "DBevents" when DBEVENTS flag is enabled,
**    for managing DBEvent on Gateway.
**  10-Nov-2004 (schph01)
**    bug 113426 Verify existence of Ice Server, before creating the sub
**    branches of ICE. When no Ice server found display the following branch:
**    < No Ice Server Running >
**  11-May-2005 (srisu02 for schph01)
**    The above fix causes errors when a Force Refresh was done on the Ice
**    nodes. Corrected the error.

********************************************************************/

//
// Includes
//
// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "domdisp.h"
#include "dbaginfo.h"
#include "error.h"
#include "winutils.h"
#include "main.h"
#include "resource.h"
#include "treelb.e"
#include "dll.h"   // ResourceString
#include "monitor.h"
#include "extccall.h"
// from main.c - for MAINLIB
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);

// defined in childfrm.cpp
extern void NotifyDomFilterChanged(HWND hwndDOMDoc, LPDOMDATA lpDomData, int iAction);
extern void NotifyRightPaneForBkTaskListUpdate(HWND hwndDOMDoc, LPDOMDATA lpDomData, int iobjecttype);

// defined in mainfrm.cpp
extern void UpdateNodeUsersList(char *nodeName);

// defined in vnitem.cpp
extern BOOL INGRESII_IsDBEventsEnabled (LPNODESVRCLASSDATAMIN lpInfo);

// defined in nodes.c
extern BOOL IsIceServerLaunched(LPCTSTR lpszNode);

//
// global variables
//

// Adds a static sub item, itself having optionnaly a dummy sub item
// Receives parent database type (DBTYPE_REGULAR, DBTYPE_DISTRIBUTED, DBTYPE_COORDINATOR)
// for star management
static DWORD AddStaticSubItem(LPDOMDATA lpDomData, DWORD idParent, UCHAR *bufParent1, UCHAR *bufParent2, UCHAR *bufParent3, int parentDbType, UCHAR *bufOwner, UCHAR *bufComplim, UCHAR *tableOwner, int otStatic, UINT idRsString, BOOL bWithDummy, BOOL bSpeedFlag);

//
// static functions definition
//
static VOID   NEAR MakeSecurityCaption(UCHAR *buf, long secNo, UCHAR *alarmName);
static VOID   NEAR MakeRelatedLevel1ObjCaption(UCHAR *buf, UCHAR *parent);
static VOID   NEAR MakeCrossSecurityCaption(UCHAR *buf, UCHAR *parent, long secNo);
static int    NEAR MergeResStatus(int iret1, int iret2);
static VOID   NEAR CleanStringsArray(LPUCHAR *array, int stringsNumber);
static int    NEAR MakeLevelAndIdsNone(int iobjecttype, int *plevel, UINT *pids_none);
static BOOL   NEAR HasParent1WithSchema(int iobjecttype);
static BOOL   NEAR HasParent2WithSchema(int iobjecttype);
static BOOL   FillReplicLocalDBInformation(int hnode, LPTREERECORD lpRecord, HWND hwndParent, int ReplicVersion);


//  NEW FUNCTIONS FOR SPEED ENHANCEMENT IN THE TREE
DWORD GetAnyAnchorId(LPDOMDATA lpDomData, int iobjecttype, int level,
                     LPUCHAR *parentstrings, DWORD previousId,
                     LPUCHAR bufPar1, LPUCHAR bufPar2, LPUCHAR bufPar3,
                     int iAction, DWORD idItem, int *piresultobjecttype,
                     int *plevel, UINT *pids_none,
                     BOOL bTryCurrent, BOOL *pbRootItem);



//
// 24/08/95 : new set of functions that allow to create all the dummy
// sub-items at the same time rather than creating them item by item
// Interest : dramatic tree speed enhancement when expanding a branch
// with a lot of items (for example: 1500 grantees for a table)
//

#define MAXIMORUM 50 // maximorum 50 * 16000 approximately...
static DWORD *DumListAnchor[MAXIMORUM];
static int    DumListArrayNum;    // which array ? ( 0 to MAXIMORUM-1)
static long   DumListIndex;       // index in DumListAnchor[DumListArrayNum]
static long   DumListTotal;       // total number of elements
static long   DumListMaxItem;     // max number of elements in one array
static unsigned short  DumListSize;    // UINT TOO BIG UNDER WINDOWS NT

//
// DumListInit:
// start list of items under which a dummy sub-item has to be added
//
static BOOL NEAR DumListInit()
{
  int cpt;

  // free previous list if forgotten
  for (cpt = 0; cpt < MAXIMORUM; cpt++)
    if (DumListAnchor[cpt]) {
      ESL_FreeMem(DumListAnchor[cpt]);
      DumListAnchor[cpt] ='\0';
    }

  // allocate new list space
  DumListSize = (unsigned short)-1;    // use unsigned trick ---> largest value
  DumListSize -= 10 * sizeof(DWORD);   // minus 10 items
  DumListSize += 1;                    // cancel the -1
  DumListAnchor[0] = (DWORD *)ESL_AllocMem(DumListSize);
  if (!DumListAnchor[0])
    return FALSE;
  DumListMaxItem = DumListSize / sizeof(DWORD);
  DumListArrayNum = 0;    // array of index 0
  DumListIndex = 0L;      // index in DumListAnchor[DumListArrayNum]
  DumListTotal = 0L;      // total number of elements
  return TRUE;
}

//
// Free the list
//
static BOOL NEAR DumListFree()
{
  int cpt;

  for (cpt = 0; cpt < MAXIMORUM; cpt++)
    if (DumListAnchor[cpt]) {
      ESL_FreeMem(DumListAnchor[cpt]);
      DumListAnchor[cpt] ='\0';
    }
  return TRUE;
}

//
// DumListAdd:
// adds an item in the list
//
static BOOL NEAR DumListAdd(DWORD idChildObj)
{
  DumListAnchor[DumListArrayNum][DumListIndex++] = idChildObj;
  if (DumListIndex < DumListMaxItem) {
    DumListTotal++;
    return TRUE;
  }
  else {
    if (DumListArrayNum < MAXIMORUM-1) {
      DumListArrayNum++;
      DumListIndex = 0;
      DumListTotal++;
      DumListAnchor[DumListArrayNum] = (DWORD *)ESL_AllocMem(DumListSize);
      if (DumListAnchor[DumListArrayNum])
        return TRUE;
      else {
        DumListFree();
        return FALSE;
      }
    }
    else {
      DumListFree();
      return FALSE;
    }
  }
}

//
// Notes the item as not to be created
//
static BOOL NEAR DumListRemove(DWORD idChildObj)
{
  long  lcpt;
  int   arrayNum;
  long  index;

  for (lcpt=0L; lcpt<DumListTotal; lcpt++) {
    arrayNum = (int) (lcpt / DumListMaxItem);
    index = lcpt - arrayNum * DumListMaxItem;
    if (DumListAnchor[arrayNum][index] == idChildObj) {
      DumListAnchor[arrayNum][index] = 0;
      return TRUE;
    }
  }
  return FALSE;   // not found
}

//
// Create the dummy sub-items and free the list
//
static BOOL NEAR DumListUseAndFree(LPDOMDATA lpDomData)
{
  long  lcpt;
  int   arrayNum;
  long  index;

  // Add dummy sub items
  for (lcpt=0L; lcpt<DumListTotal; lcpt++) {
    arrayNum = (int) (lcpt / DumListMaxItem);
    index = lcpt - arrayNum * DumListMaxItem;

    // check item not removed
    if (DumListAnchor[arrayNum][index] == 0L)
      continue;

    // create sub item
    if (AddDummySubItem(lpDomData, DumListAnchor[arrayNum][index]) == 0) {
      // Error occured
      // force full collapse mode to get the tree recalculated
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)DumListAnchor[arrayNum][index]);
      // free the list and return error
      DumListFree();
      return FALSE;
      break;
    }
  }

  // Set the parent items in collapsed mode
  // with speed flag except for last item
  for (lcpt=0L; lcpt<DumListTotal; lcpt++) {
    arrayNum = (int) (lcpt / DumListMaxItem);
    index = lcpt - arrayNum * DumListMaxItem;

    // check item not removed
    if (DumListAnchor[arrayNum][index] == 0L)
      continue;

    SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                (lcpt < DumListIndex-1 ? 0xdcba : 0),
                (LPARAM)DumListAnchor[arrayNum][index]); // 0xdcba : speed
  }

  DumListFree();
  return TRUE;
}

//
//  int DOMDisableDisplay(int node, HWND hwndDOMDoc)
//
//  Disables display refreshes for the tree referenced by hwndDOMDoc,
//  or for all trees referencing the node if hwndDOMDoc is NULL
//
//  return value : to be defined
//
//  Modified Oct 2., 95 : do nothing since critical section code
//  takes care of Enable/Disable and Focus; simply manage the cursor
//
int DOMDisableDisplay(int hnodestruct, HWND hwndDOMDoc)
{
	// to be changed (using Notify_MT_Action) if anything is added in the future in the base function
	// we don't even want the hourglass (invoked at the end of the base function) if we are in the secondary thread 
	if (CanBeInSecondaryThead()) 
		return 1;

#ifdef OBSOLETE

  LPDOMDATA lpDomData;
  HWND      hwndCurDoc;       // for loop on all documents

  // loop on documents if hwndDOMDoc null
  if (!hwndDOMDoc) {
    // get first document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc) {
      // manage update for this mdi document if concerned with
      if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) {
        lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
        if (lpDomData->psDomNode->nodeHandle == hnodestruct)
          DOMDisableDisplay(hnodestruct, hwndCurDoc);
      }
      // get next document handle (with loop to skip the icon title windows)
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
      while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
        hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    }
    return 1;
  }

  // Disable keyboard and mouse input
  lpDomData = (LPDOMDATA)GetWindowLong(hwndDOMDoc, 0);
  EnableWindow(lpDomData->hwndTreeLb, FALSE);

#endif    // OBSOLETE

  // Change cursor to hourglass
  ShowHourGlass();

  return 1;
}

//
//  int DOMEnableDisplay(int node, HWND hwndDOMDoc)
//
//  Enables display refreshes for the tree referenced by hwndDOMDoc,
//  or for all trees referencing the node if hwndDOMDoc is NULL
//
//  return value : to be defined
//
//  Modified Oct 2., 95 : do nothing since critical section code
//  takes care of Enable/Disable and Focus; simply manage the cursor
//
int DOMEnableDisplay(int hnodestruct, HWND hwndDOMDoc)
{

	// to be changed (using Notify_MT_Action) if anything is added in the future in the base function
	// there shouldn't be the hourglass (removed at the end of the base function) if we are in the secondary thread 
	if (CanBeInSecondaryThead()) 
		return 1;

#ifdef OBSOLETE

  LPDOMDATA lpDomData;
  HWND      hwndCurDoc;       // for loop on all documents
  HWND      hwndFocus;

  // loop on documents if hwndDOMDoc null
  if (!hwndDOMDoc) {
    // get first document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc) {
      // manage update for this mdi document if concerned with
      if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) {
        lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
        if (lpDomData->psDomNode->nodeHandle == hnodestruct)
          DOMEnableDisplay(hnodestruct, hwndCurDoc);
      }
      // get next document handle (with loop to skip the icon title windows)
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
      while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
        hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    }
    return 1;
  }

  // Enable keyboard and mouse input
  lpDomData = (LPDOMDATA)GetWindowLong(hwndDOMDoc, 0);
  EnableWindow(lpDomData->hwndTreeLb, TRUE);
  hwndFocus = GetFocus();
  if (hwndFocus == NULL || hwndFocus == hwndFrame)
    SetFocus(lpDomData->hwndTreeLb);

  // Update the tree - NOT NECESSARY - TO BE DISCUSSED/FINISHED
  //InvalidateRect(lpDomData->hwndTreeLb, NULL, TRUE);
  //UpdateWindow(lpDomData->hwndTreeLb);

#endif    // OBSOLETE

  // Restore cursor from hourglass
  RemoveHourGlass();

  return 1;
}

//
//  Does the level 1 parent of this object have a schema ?
//
static BOOL NEAR HasParent1WithSchema(int iobjecttype)
{
  switch (iobjecttype) {
    case OT_INTEGRITY:
    case OT_RULE:
    case OT_RULEPROC:
    case OT_INDEX:
    case OT_TABLELOCATION:
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:
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
    case OT_DBEGRANT_RAISE_USER:
    case OT_DBEGRANT_REGTR_USER:
    case OT_PROCGRANT_EXEC_USER:
    case OT_SEQUGRANT_NEXT_USER:
    case OTR_TABLESYNONYM:
    case OTR_INDEXSYNONYM:
    case OTR_TABLEVIEW:
    case OTR_PROC_RULE:
    case OTR_REPLIC_TABLE_CDDS:

    // views also can have schemas!
    case OT_VIEWGRANT_SEL_USER:
    case OT_VIEWGRANT_INS_USER:
    case OT_VIEWGRANT_UPD_USER:
    case OT_VIEWGRANT_DEL_USER:
    case OT_VIEWTABLE:            // view component
    case OTR_VIEWSYNONYM:         // view synonym

      return TRUE;
  }
  return FALSE;   // default
}

//
//  Does the level 2 parent of this object have a schema ?
//
static BOOL NEAR HasParent2WithSchema(int iobjecttype)
{
  switch (iobjecttype) {
    case OTR_INDEXSYNONYM:
    case OT_RULEPROC:
      return TRUE;
  }
  return FALSE;   // default
}

//
//  Does DOMGetFirst/Next return something useful in the "object owner"
//  field, for an object that has no owner ?
//
static BOOL NEAR ObjectUsesOwnerField(int iobjecttype)
{
  switch (iobjecttype) {
    // New style Alarms for tables and for databases
    // bufOwner ---> schema.dbevent associated with the alarm
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:

    // Replicator
    case OT_REPLIC_CONNECTION:    // connection security number in bufOwner
    case OT_REPLIC_CDDS:          // CDDS number in bufOwner

      return TRUE;
  }
  return FALSE;
}


// Problems when changing from system to not system
extern void MfcBlockUpdateRightPane();
extern void MfcAuthorizeUpdateRightPane();

int  DOMUpdateDisplayData (int      hnodestruct,    // handle on node struct
                           int      iobjecttype,    // object type
                           int      level,          // parenthood level
                           LPUCHAR *parentstrings,  // parenthood names
                           BOOL     bWithChildren,  // should we expand children?
                           int      iAction,        // why is this function called ?
                           DWORD    idItem,         // if expansion: item being expanded
                           HWND     hwndDOMDoc)     // handle on DOM MDI document
{
	if (CanBeInSecondaryThead()) {
		DOMUPDATEDISPLAYDATA params;
		params.hnodestruct   = hnodestruct;
		params.iobjecttype   = iobjecttype;
		params.level         = level;
		params.parentstrings = parentstrings;
		params.bWithChildren = bWithChildren;
		params.iAction       = iAction;
		params.idItem        = idItem;
		params.hwndDOMDoc    = hwndDOMDoc;
		return (int) Notify_MT_Action(ACTION_DOMUPDATEDISPLAYDATA, (LPARAM) &params);// for being executed in the main thread
	}
	else
		return DOMUpdateDisplayData_MT (hnodestruct,    // handle on node struct
                                       iobjecttype,    // object type
                                       level,          // parenthood level
                                       parentstrings,  // parenthood names
                                       bWithChildren,  // should we expand children?
                                       iAction,        // why is this function called ?
                                       idItem,         // if expansion: item being expanded
                                       hwndDOMDoc);    // handle on DOM MDI document
}
// Trick 16/3/95 given of Fnn not to change the calls in all sources

int  DOMUpdateDisplayData_MT (int      hnodestruct,    // handle on node struct
                           int      iobjecttype,    // object type
                           int      level,          // parenthood level
                           LPUCHAR *parentstrings,  // parenthood names
                           BOOL     bWithChildren,  // should we expand children?
                           int      iAction,        // why is this function called ?
                           DWORD    idItem,         // if expansion: item being expanded
                           HWND     hwndDOMDoc)     // handle on DOM MDI document
{
  int forceRefreshMode = FORCE_REFRESH_NONE;
  int retval;
  LPDOMDATA lpDomData = NULL;
  BOOL bMustBlock = FALSE;
  DWORD dwOldCurSel = 0;
  DWORD dwNewCurSel = 0;

  if (hwndDOMDoc)
    lpDomData = (LPDOMDATA)GetWindowLong(hwndDOMDoc, 0);

  switch (iAction) {
    case ACT_CHANGE_SYSTEMOBJECTS:
    case ACT_CHANGE_BASE_FILTER:
    case ACT_CHANGE_OTHER_FILTER:
    case ACT_BKTASK:
      bMustBlock = TRUE;
      break;
    default:
      bMustBlock = FALSE;
      break;
  }

  if (bMustBlock && lpDomData) {
    dwOldCurSel = GetCurSel(lpDomData);
    MfcBlockUpdateRightPane();
  }
  retval = DOMUpdateDisplayData2 (hnodestruct, iobjecttype, level, parentstrings, bWithChildren, iAction, idItem, hwndDOMDoc, forceRefreshMode);
  if (bMustBlock && lpDomData) {
    MfcAuthorizeUpdateRightPane();
    dwNewCurSel = GetCurSel(lpDomData);
    if (dwNewCurSel != dwOldCurSel) {
      UpdateRightPane(hwndDOMDoc, lpDomData, TRUE, 0);
      lpDomData->dwSelBeforeRClick = dwNewCurSel;
    }
  }

  // Fix Nov 5, 97: must Update number of objects after actions
  // on toolbar (system object checkbox and filter comboboxes
  // (necessary since we have optimized the function that counts the tree items
  // for performance purposes)
  switch (iAction) {
    case ACT_CHANGE_SYSTEMOBJECTS:
    case ACT_CHANGE_BASE_FILTER:
    case ACT_CHANGE_OTHER_FILTER:
    case ACT_BKTASK:
      if (lpDomData)  // can be null in case of background refresh (hwndDOMDoc null)
        GetNumberOfObjects(lpDomData, TRUE);
        // display will be updated at next call to the same function
      break;
  }

  // Complimentary for right pane notification
  switch (iAction) {
    case ACT_CHANGE_SYSTEMOBJECTS:
    case ACT_CHANGE_BASE_FILTER:
    case ACT_CHANGE_OTHER_FILTER:
      if (lpDomData)
        NotifyDomFilterChanged(hwndDOMDoc, lpDomData, iAction);
      break;

    case ACT_BKTASK:
      if (hwndDOMDoc && lpDomData)
        NotifyRightPaneForBkTaskListUpdate(hwndDOMDoc, lpDomData, iobjecttype);
      break;
  }

  return retval;
}


int UpdateIceDisplayBranch( HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD dwFirstItem,dwNextItem,dwChildObj,recId;
  int iRet, objType;
  char szVnodeName[BUFSIZE];
  BOOL bServerLaunched;

  iRet = RES_SUCCESS;
  // loop on the level 0 branches
  // search ICE branch
  dwFirstItem = GetFirstLevel0Item(lpDomData);
  dwNextItem = dwFirstItem;
  while (dwNextItem) {
    objType = GetItemObjType(lpDomData, dwNextItem);
    if (objType == OT_STATIC_ICE)
    {
       recId = dwNextItem;
       break;
    }
    dwNextItem = GetNextLevel0Item(lpDomData, dwNextItem);
  }

  dwChildObj = GetFirstImmediateChild(lpDomData, recId, 0);
  if (!dwChildObj)
  {
    return iRet; // no Sub-branch
  }
  objType = GetItemObjType(lpDomData, dwChildObj);
  if (objType == OT_STATIC_DUMMY)
    return iRet; // Ice branches not expanded. No refresh needed

  // ICE branch already Expanded
  lstrcpy(szVnodeName,GetVirtNodeName (lpDomData->psDomNode->nodeHandle));
  RemoveConnectUserFromString(szVnodeName);
  RemoveGWNameFromString(szVnodeName);
  bServerLaunched = IsIceServerLaunched(szVnodeName); //retrieve the state of ICE server

  if (objType == OT_STATIC_ICE_SECURITY && bServerLaunched)
      return iRet; // Same state during the previous Ice Branch expand.
  if (objType == OT_ICE_SUBSTATICS && !bServerLaunched )
      return iRet; // Same state during the previous Ice Branch expand.

  // the state of ICE server change, refresh need.
  gMemErrFlag = MEMERR_NOMESSAGE;
  DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, 0);

  iRet = DOMUpdateDisplayData2 (
                   lpDomData->psDomNode->nodeHandle,       // handle on node struct
                   OT_ICE_SUBSTATICS, // object type
                   0,                 // parenthood level
                   NULL,              // parenthood names
                   FALSE,             // should we expand children?
                   ACT_EXPANDITEM,    // why is this function called ?
                   recId,             // if expansion: item being expanded
                   hwndMdi,           // handle on DOM MDI document
                   FORCE_REFRESH_NONE);
  DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);

  return iRet;
}
//
// int DOMUpdateDisplayData2 (
//        int      hnodestruct,     // handle on node struct
//        int      iobjecttype,     // object type
//        int      level,           // parenthood level
//        LPUCHAR *parentstrings,   // parenthood names
//        BOOL     bWithChildren,   // should we expand children?
//        int      iAction,         // why is this function called ?
//        DWORD    idItem,          // if expansion: item being expanded
//        HWND     hwndDOMDoc,      // handle on DOM MDI document
//        int      forceRefreshMode);
//
//  Updates the tree referenced by hwndDOMDoc for the object type given
//  or for all trees referencing the node if hwndDOMDoc is NULL
//
//  MUST be framed with DOMEnableDisplay / DOMDisableDisplay
//  to avoid screen flash effects
//
//  return value : RES_xxx
//
//
int DOMUpdateDisplayData2 (int hnodestruct, int iobjecttype, int level, LPUCHAR *parentstrings, BOOL bWithChildren, int iAction, DWORD idItem, HWND hwndDOMDoc, int forceRefreshMode)
{
  int           iret;
  int           iretGlob;
  DWORD         idAnchorObj;      // id of the anchor object in the lbtree
  DWORD         idChildObj;       // id of the child object in the lbtree
  DWORD         idChild2Obj;      // id of the child objects of second level
  DWORD         idChild3Obj;      // id of child objects of depper level
  DWORD         idChild4Obj;      // id of child objects of ultra-deep level
  DWORD         idFirstOldChildObj; // id of the child in the branch
                                    // FindImmediateSubItem will start of
  DWORD         idLastOldChildObj;  // id of the child in the branch
                                    // FindImmediateSubItem will stop on
  DWORD         idInsertAfter;    // child we will insert after, for sort
  DWORD         idInsertBefore;   // child we will insert before, for sort
  UCHAR         buf[BUFSIZE];
  UCHAR         buf2[BUFSIZE];
  UCHAR         buf3[BUFSIZE];
  UCHAR         buf4[BUFSIZE];
  UCHAR         bufOwner[MAXOBJECTNAME];
  UCHAR         bufComplim[MAXOBJECTNAME];
  LPUCHAR       aparents[MAXPLEVEL];      // array of parent strings
  LPUCHAR       aparentsTemp[MAXPLEVEL];  // temporary aparents
  UCHAR         bufPar0[MAXOBJECTNAME];   // parent string of level 0
  UCHAR         bufPar1[MAXOBJECTNAME];   // parent string of level 1
  UCHAR         bufPar2[MAXOBJECTNAME];   // parent string of level 2
  UCHAR         bufRs[BUFSIZE];           // resource string
  LPDOMDATA     lpDomData;
  LPTREERECORD  lpRecord  = NULL;
  LPTREERECORD  lpRecord2;          // in case we must keep lpRecord in mind
  LPTREERECORD  lpRecord4WildCard; 
  HWND          hwndCurDoc;         // for loop on all documents
  UINT          ids_none;
  BOOL          bwithsystem;        // for DOMGetFirstObject
  LPUCHAR       lpowner;            // for DOMGetFirstObject
  DWORD         idTop;              // Top item, for expand display problem
  LPUCHAR       aparentsResult[MAXPLEVEL];  // result aparents for OTR_ obj.
  UCHAR         bufParResult0[MAXOBJECTNAME];   // result parent level 0
  UCHAR         bufParResult1[MAXOBJECTNAME];   // result parent level 1
  UCHAR         bufParResult2[MAXOBJECTNAME];   // result parent level 2
  int           resultType;
  int           expandType;
  int           resultLevel;
  BOOL          bNoItem;            // TRUE if no item at all
  LPUCHAR       parent0;            // ptr on parent of level 0, for find fct
  SETSTRING     setString;          // for type solve tree line caption/replicator
  int           basicType;          // for gray items management

  BOOL          bUser;              // is this a user ?

  #ifdef DEBUGMALLOC
  int           itemCpt = 0;        // item count, for breakpoint only
  #endif  //DEBUGMALLOC

  // Several branches to update at one time : OT_VIRTNODE, OTLL_xxx
  BOOL    bMultipleUpdates;         // Do we have several branches to update?
  int     irefobjecttype;           // reference requested type

  DWORD   anchorLevel;              // tree speed optimization
  int     nReplVer;                 // PS for version of replication

  // for DOMGetObject
  LPUCHAR aparentsResult2[MAXPLEVEL];
  UCHAR   bufParResult02[MAXOBJECTNAME];
  UCHAR   bufParResult12[MAXOBJECTNAME];
  UCHAR   bufParResult22[MAXOBJECTNAME];

  // for root item management
  BOOL    bRootItem;     // non-static item as a root on the tree?

  // for speed optimize
  LPTREERECORD  lpAnchorRecord = NULL;
  BOOL    bOldCode = FALSE; // if set to TRUE with debugger, old code behaviour
  time_t  time1, time2, time3, time4; // test only

  // for desktop
  BOOL bSubDummy;

  // Star management
  int  databaseType = -1;   // star...
  int  objType      = -1;   // star...

  // Language used to generate the view TRUE = SQL , FALSE = QUEL
  BOOL bViewSqlType;
  // for interruptible multithreaded refresh
  int retRefresh = RES_ERR;

#ifdef WIN32
  // for bug in tree dll
  // masqued 30/05/97 since generic fix by emb in tree dll
  // DWORD idAntiSmutObj;      // line inserted prior to others
#endif // WIN32

  // default return value
  iretGlob = RES_SUCCESS;

  // anchor aparentsResult on buffers
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;

  // Manage forceRefreshMode - PRIOR TO LOOP ON HWNDDOMDOC
  if (forceRefreshMode != FORCE_REFRESH_NONE) {
    if (hwndDOMDoc == 0)
      return RES_ERR;   // FATAL: we MUST have received a valid doc handle
    lpDomData = (LPDOMDATA)GetWindowLong(hwndDOMDoc, 0);
    bwithsystem = lpDomData->bwithsystem;
    switch (forceRefreshMode) {
      case FORCE_REFRESH_ALLBRANCHES:
        PrepareGlobalDOMUpd(FALSE); // TO BE FINISHED : manage retval 0
        retRefresh = ManageForceRefreshAll(hwndDOMDoc, lpDomData, NULL);   // last parameter for multithreaded purposes
        /*if (retRefresh == RES_ERR)
          MessageBox(GetFocus(),
                     "Refresh Interrupted by user action",
                     "Refresh All Branches",
                      MB_OK | MB_ICONEXCLAMATION);
        */
        DOMUpdateDisplayData (
                lpDomData->psDomNode->nodeHandle, // hnodestruct
                OT_VIRTNODE,                      // iobjecttype
                0,                                // level
                NULL,                             // parentstrings
                FALSE,                            // bWithChildren
                ACT_BKTASK,                       // all must be refreshed
                0L,                               // no item id
                0);                               // all doms on the node
        return RES_SUCCESS;
        break;

    }
  }


  // if hwndDOMDoc null : loop on DOM mdi documents with the same node
  if (!hwndDOMDoc) {
    // get first document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc) {
      // manage update for this mdi document if concerned with
      if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) {
        HWND hwndMfcDoc = GetVdbaDocumentHandle(hwndCurDoc);
        lpDomData = (LPDOMDATA)GetWindowLong(hwndMfcDoc, 0);
        if (lpDomData->psDomNode->nodeHandle == hnodestruct) {
          iret = DOMUpdateDisplayData2 (hnodestruct, iobjecttype,
                                      level, parentstrings,
                                      bWithChildren, iAction,
                                      idItem, hwndMfcDoc,
                                      forceRefreshMode);
          iretGlob = MergeResStatus(iretGlob, iret);

          // complimentary emb for right pane bktask update - will only update panes of type "List", i.e. GetFirst/Next
          if (iAction == ACT_BKTASK)
            NotifyRightPaneForBkTaskListUpdate(hwndMfcDoc, lpDomData, iobjecttype);
        }
      }
      // get next document handle (with loop to skip the icon title windows)
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
      while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
        hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    }
    return iretGlob;
  }

  // Must be done after we solved the hwndDOMDoc null case
  lpDomData = (LPDOMDATA)GetWindowLong(hwndDOMDoc, 0);
  bwithsystem = lpDomData->bwithsystem;

  switch (iobjecttype) {

    //
    // ON-THE-FLY Object type solve
    //

    // pseudo level 0: grantee can be a user, a group or a role
    case OT_GRANTEE_SOLVE:
      // 0) memorize top item
      idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                 0, 0L);

      // 1) check parents : no check since level 0

      // 2) Get real object type and info
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)idItem);
      if (!lpRecord)
        return RES_ERR;

      // re-anchor aparentsResult on buffers
      aparentsResult[0] = bufParResult0;
      aparentsResult[1] = bufParResult1;
      aparentsResult[2] = bufParResult2;
      CleanStringsArray(aparentsResult,
                        sizeof(aparentsResult)/sizeof(aparentsResult[0]));
      memset (&bufComplim, '\0', sizeof(bufComplim));
      memset (&bufOwner, '\0', sizeof(bufOwner));

      iret = DOMGetObject(hnodestruct,
                          lpRecord->objName,
                          lpRecord->ownerName,
                          OT_GRANTEE,
                          0,              // level
                          NULL,           // aparents
                          bwithsystem,
                          &resultType,
                          &resultLevel,
                          aparentsResult, // array of result parent strings
                          buf,            // result object name
                          bufOwner,       // result owner
                          bufComplim);    // result complimentary data
      // clear bufOwner and bufComplim AFTER call - TO BE FINISHED
      if (resultType!= OT_DATABASE && !NonDBObjectHasOwner(resultType))
        bufOwner[0] = '\0';

      if (iret == RES_ENDOFDATA) {
        // object has previously been deleted!

        // set item back in collapsed mode
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idItem);

        // Message box for end user
        LoadString(hResource, IDS_TREE_GRANTEE_NONEXISTENT, bufRs, sizeof(bufRs));
        MessageBox(hwndDOMDoc, bufRs, NULL,
                   MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

        // finished
        return RES_SUCCESS;
      }

      if (iret != RES_SUCCESS)
        return RES_ERR;

      // 2bis) change resultType to keep the "grantee" state
      switch (resultType) {
        case OT_USER:
          resultType = OT_GRANTEE_SOLVED_USER;
          break;
        case OT_GROUP:
          resultType = OT_GRANTEE_SOLVED_GROUP;
          break;
        case OT_ROLE:
          resultType = OT_GRANTEE_SOLVED_ROLE;
          break;
      }

      // 3) Update the object
      // MASQUED 7/4/95: Don't update the object type, neither the parenthood
      // lpRecord->recType = resultType;
      // lstrcpy(lpRecord->extra,      aparentsResult[0]);
      // lstrcpy(lpRecord->extra2,     aparentsResult[1]);
      // lstrcpy(lpRecord->extra3,     aparentsResult[2]);

      // Added for fnn request 16/05/97 :
      lpRecord->displayType = resultType;

      lstrcpy(lpRecord->objName,    buf);
      lstrcpy(lpRecord->ownerName,  bufOwner);
      lstrcpy(lpRecord->szComplim,  bufComplim);

      // 4) update the status bar : automatic

      // 5) self-call for expansion with the right type,
      // or local generation of sub-items
      switch (resultType) {
        case OT_GRANTEE_SOLVED_USER:
          expandType = OT_USER_SUBSTATICS;
          break;

        case OT_GRANTEE_SOLVED_GROUP:
          // save dummy sub-item id for deletion
          idChildObj = GetFirstImmediateChild(lpDomData, idItem, 0);

          // Add sub-subitem GROUPUSERS with sub dummy collapsed
          // we put the group name in the extra data of the record
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_GROUPUSER,
                        IDS_TREE_GROUPUSER_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the group name in the extra data of the record
          // we delay the sub-items creation at expand time
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // delete dummy sub item
          TreeDeleteRecord(lpDomData, idChildObj);

          // say item is valid
          lpRecord->bSubValid = TRUE;

          return MergeResStatus(iretGlob, iret);
          break;

        case OT_GRANTEE_SOLVED_ROLE:
          // save dummy sub-item id for deletion
          idChildObj = GetFirstImmediateChild(lpDomData, idItem, 0);

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the role name in the extra data of the record
          // we delay the sub-items creation at expand time
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // delete dummy sub item
          TreeDeleteRecord(lpDomData, idChildObj);

          // say item is valid
          lpRecord->bSubValid = TRUE;

          return MergeResStatus(iretGlob, iret);
          break;

        default:
          expandType = 0;
      }
      if (!expandType)
        return MergeResStatus(iretGlob, RES_ERR);   // Unexpected type!

      iret = DOMUpdateDisplayData2(hnodestruct,
                                  expandType,
                                  resultLevel,
                                  aparentsResult,
                                  FALSE,                // bwithchildren
                                  iAction,
                                  idItem,
                                  hwndDOMDoc,
                                  forceRefreshMode);

      // set back top item
      SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

      return MergeResStatus(iretGlob, iret);
      break;

    // pseudo level 0: alarmee can be a user, a group or a role
    case OT_ALARMEE_SOLVE:
      // 0) memorize top item
      idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                 0, 0L);

      // 1) check parents : no check since level 0

      // 2) Get real object type and info
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)idItem);
      if (!lpRecord)
        return RES_ERR;

      // re-anchor aparentsResult on buffers
      aparentsResult[0] = bufParResult0;
      aparentsResult[1] = bufParResult1;
      aparentsResult[2] = bufParResult2;
      CleanStringsArray(aparentsResult,
                        sizeof(aparentsResult)/sizeof(aparentsResult[0]));
      memset (&bufComplim, '\0', sizeof(bufComplim));
      memset (&bufOwner, '\0', sizeof(bufOwner));

      iret = DOMGetObject(hnodestruct,
                          lpRecord->objName,
                          lpRecord->ownerName,
                          OT_GRANTEE,     // OT_ALARMEE not known
                          0,              // level
                          NULL,           // aparents
                          bwithsystem,
                          &resultType,
                          &resultLevel,
                          aparentsResult, // array of result parent strings
                          buf,            // result object name
                          bufOwner,       // result owner
                          bufComplim);    // result complimentary data

      if (iret == RES_ENDOFDATA) {
        // object has previously been deleted!

        // set item back in collapsed mode
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idItem);

        // Message box for end user
        LoadString(hResource, IDS_TREE_ALARMEE_NONEXISTENT, bufRs, sizeof(bufRs));
        MessageBox(hwndDOMDoc, bufRs, NULL,
                   MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

        // finished
        return RES_SUCCESS;
      }

      if (iret != RES_SUCCESS)
        return RES_ERR;

      // 2bis) change resultType to keep the "grantee" state
      switch (resultType) {
        case OT_USER:
          resultType = OT_ALARMEE_SOLVED_USER;
          break;
        case OT_GROUP:
          resultType = OT_ALARMEE_SOLVED_GROUP;
          break;
        case OT_ROLE:
          resultType = OT_ALARMEE_SOLVED_ROLE;
          break;
      }

      // 3) Update the object
      // MASQUED 7/4/95: Don't update the object type, neither the parenthood
      // lpRecord->recType = resultType;
      // lstrcpy(lpRecord->extra,      aparentsResult[0]);
      // lstrcpy(lpRecord->extra2,     aparentsResult[1]);
      // lstrcpy(lpRecord->extra3,     aparentsResult[2]);

      // Added for fnn request 16/05/97 :
      lpRecord->displayType = resultType;

      lstrcpy(lpRecord->objName,    buf);
      lstrcpy(lpRecord->ownerName,  bufOwner);
      lstrcpy(lpRecord->szComplim,  bufComplim);

      // 4) update the status bar : automatic

      // 5) self-call for expansion with the right type,
      // or local generation of sub-items
      switch (resultType) {
        case OT_ALARMEE_SOLVED_USER:
          expandType = OT_USER_SUBSTATICS;
          break;

        case OT_ALARMEE_SOLVED_GROUP:
          // save dummy sub-item id for deletion
          idChildObj = GetFirstImmediateChild(lpDomData, idItem, 0);

          // Add sub-subitem GROUPUSERS with sub dummy collapsed
          // we put the group name in the extra data of the record
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_GROUPUSER,
                        IDS_TREE_GROUPUSER_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the group name in the extra data of the record
          // we delay the sub-items creation at expand time
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // delete dummy sub item
          TreeDeleteRecord(lpDomData, idChildObj);

          // say item is valid
          lpRecord->bSubValid = TRUE;

          return MergeResStatus(iretGlob, iret);
          break;

        case OT_ALARMEE_SOLVED_ROLE:
          // save dummy sub-item id for deletion
          idChildObj = GetFirstImmediateChild(lpDomData, idItem, 0);

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the role name in the extra data of the record
          // we delay the sub-items creation at expand time
          idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName,
                        lpRecord->szComplim,
                        NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // delete dummy sub item
          TreeDeleteRecord(lpDomData, idChildObj);

          // say item is valid
          lpRecord->bSubValid = TRUE;

          return MergeResStatus(iretGlob, iret);
          break;

        default:
          expandType = 0;
      }
      if (!expandType)
        return MergeResStatus(iretGlob, RES_ERR);   // Unexpected type!

      iret = DOMUpdateDisplayData2(hnodestruct,
                                  expandType,
                                  resultLevel,
                                  aparentsResult,
                                  FALSE,                // bwithchildren
                                  iAction,
                                  idItem,
                                  hwndDOMDoc,
                                  forceRefreshMode);

      // set back top item
      SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

      return MergeResStatus(iretGlob, iret);
      break;

    // pseudo level 2, child of "view of database"
    case OT_VIEWTABLE_SOLVE:
      // 0) memorize top item
      idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                 0, 0L);

      // 1) check parents --- MANDATORY!
      if (parentstrings==NULL || parentstrings[0]==NULL
                              || parentstrings[0][0]=='\0')
        return MergeResStatus(iretGlob, RES_ERR);
      aparents[0] = parentstrings[0];   // base name

      // 2) Get real object type and info
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)idItem);
      if (!lpRecord)
        return RES_ERR;

      // re-anchor aparentsResult on buffers
      aparentsResult[0] = bufParResult0;
      aparentsResult[1] = bufParResult1;
      aparentsResult[2] = bufParResult2;
      CleanStringsArray(aparentsResult,
                        sizeof(aparentsResult)/sizeof(aparentsResult[0]));
      memset (&bufComplim, '\0', sizeof(bufComplim));
      memset (&bufOwner, '\0', sizeof(bufOwner));

      iret = DOMGetObject(hnodestruct,
                          lpRecord->objName,
                          lpRecord->ownerName,
                          OT_VIEWTABLE,
                          level,
                          aparents,
                          bwithsystem,
                          &resultType,
                          &resultLevel,
                          aparentsResult, // array of result parent strings
                          buf,            // result object name
                          bufOwner,       // result owner
                          bufComplim);    // result complimentary data
      // clear bufOwner and bufComplim AFTER call - TO BE FINISHED
      if (resultType!= OT_DATABASE && !NonDBObjectHasOwner(resultType))
        bufOwner[0] = '\0';

      if (iret == RES_ENDOFDATA) {
        // object has previously been deleted!

        // set item back in collapsed mode
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idItem);

        // Message box for end user
        if (GetOIVers()==OIVERS_NOTOI) {
         MessageBox(hwndDOMDoc, "Cannot Expand Object", NULL,
                     MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        }
        else {
         LoadString(hResource, IDS_TREE_VIEWTABLE_NONEXISTENT, bufRs, sizeof(bufRs));
         MessageBox(hwndDOMDoc, bufRs, NULL,
                     MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        }

        // prepare parameters starting from the parent branch
        idAnchorObj = (DWORD)SendMessage(lpDomData->hwndTreeLb,
                                         LM_GETPARENT, 0, (LPARAM)idItem);
        if (!idAnchorObj)
          return RES_ERR;
        lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                              LM_GETITEMDATA,
                                              0, (LPARAM)idItem);
        if (!lpRecord)
          return RES_ERR;
        level = 2;
        aparents[0] = lpRecord->extra;    // base name
        aparents[1] = lpRecord->extra2;   // view name
        aparents[2] = NULL;

        // Call UpdateDOMData
        UpdateDOMData(hnodestruct,
                      OT_VIEWTABLE,
                      level,          // should be 2
                      aparents,       // should contain 2 valid strings
                      bwithsystem,
                      FALSE);

        // Self call at parent level to refresh the list
        iret = DOMUpdateDisplayData2(hnodestruct,
                                    OT_VIEWTABLE,
                                    level,              // level
                                    aparents,           // aparents
                                    FALSE,              // bwithchildren
                                    ACT_EXPANDITEM,
                                    idAnchorObj,
                                    hwndDOMDoc,
                                    forceRefreshMode);

        // return the merged result
        return MergeResStatus(iretGlob, iret);
      }

      if (iret != RES_SUCCESS)
        return RES_ERR;

      // 3) Update the object
      lpRecord->recType = resultType;
      lstrcpy(lpRecord->extra,      aparentsResult[0]);
      lstrcpy(lpRecord->extra2,     aparentsResult[1]);
      lstrcpy(lpRecord->extra3,     aparentsResult[2]);
      lstrcpy(lpRecord->objName,    buf);
      lstrcpy(lpRecord->ownerName,  bufOwner);
      // for Star: must preserve all data in bufComplim (distrib. type)
      memcpy(lpRecord->szComplim, bufComplim, sizeof(lpRecord->szComplim));
      // old code: lstrcpy(lpRecord->szComplim,  bufComplim);

      // 4) update the status bar : automatic

      // 5) self-call for expansion with the right type
      switch (resultType) {
        case OT_TABLE:
          expandType = OT_TABLE_SUBSTATICS;
          break;
        case OT_VIEW:
          expandType = OT_VIEW_SUBSTATICS;
          break;
        default:
          expandType = 0;
      }
      if (!expandType)
        return MergeResStatus(iretGlob, RES_ERR);   // Unexpected type!

      iret = DOMUpdateDisplayData2(hnodestruct,
                                  expandType,
                                  resultLevel,
                                  aparentsResult,
                                  FALSE,                // bwithchildren
                                  iAction,
                                  idItem,
                                  hwndDOMDoc,
                                  forceRefreshMode);

      // set back top item
      SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

      return MergeResStatus(iretGlob, iret);
      break;

    // pseudo level 1: synonymed object can be a table, a view or an index
    case OT_SYNONYMED_SOLVE:
      // 0) memorize top item
      idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                 0, 0L);

      // 1) check parents --- MANDATORY!
      if (parentstrings==NULL || parentstrings[0]==NULL
                              || parentstrings[0][0]=='\0')
        return MergeResStatus(iretGlob, RES_ERR);
      aparents[0] = parentstrings[0];   // base name

      // 2) Get real object type and info
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                            LM_GETITEMDATA,
                                            0, (LPARAM)idItem);
      if (!lpRecord)
        return RES_ERR;

      // re-anchor aparentsResult on buffers
      aparentsResult[0] = bufParResult0;
      aparentsResult[1] = bufParResult1;
      aparentsResult[2] = bufParResult2;
      CleanStringsArray(aparentsResult,
                        sizeof(aparentsResult)/sizeof(aparentsResult[0]));
      memset (&bufComplim, '\0', sizeof(bufComplim));
      memset (&bufOwner, '\0', sizeof(bufOwner));

      iret = DOMGetObject(hnodestruct,
                          lpRecord->objName,
                          lpRecord->ownerName,
                          OT_SYNONYMOBJECT,
                          level,            // level
                          aparents,         // aparents[0] : base name
                          bwithsystem,
                          &resultType,
                          &resultLevel,
                          aparentsResult, // array of result parent strings
                          buf,            // result object name
                          bufOwner,       // result owner
                          bufComplim);    // result complimentary data
      // clear bufOwner and bufComplim AFTER call - TO BE FINISHED
      if (resultType!= OT_DATABASE && !NonDBObjectHasOwner(resultType))
        bufOwner[0] = '\0';

      if (iret == RES_ENDOFDATA) {
        // object has previously been deleted!

        // set item back in collapsed mode
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idItem);

        // Message box for end user
        LoadString(hResource, IDS_TREE_SYNONYMED_NONEXISTENT, bufRs, sizeof(bufRs));
        MessageBox(hwndDOMDoc, bufRs, NULL,
                   MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);

        // finished
        return RES_SUCCESS;
      }

      if (iret != RES_SUCCESS) {
        // set item back in collapsed mode
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idItem);
        return RES_ERR;
      }

      // 3) Update the object
      lpRecord->recType = resultType;
      lstrcpy(lpRecord->extra,      aparentsResult[0]);
      lstrcpy(lpRecord->extra2,     aparentsResult[1]);
      lstrcpy(lpRecord->extra3,     aparentsResult[2]);
      lstrcpy(lpRecord->objName,    buf);
      lstrcpy(lpRecord->ownerName,  bufOwner);
      lstrcpy(lpRecord->szComplim,  bufComplim);

      // 3bis) If the object type is an index, update his parent table schema
      if (resultType == OT_INDEX) {
        // TO BE FINISHED - SCHEMA ???
      }

      // 4) update the status bar : automatic

      // 5) self-call for expansion with the right type,
      // or local generation of sub-items
      switch (resultType) {
        case OT_TABLE:
          expandType = OT_TABLE_SUBSTATICS;
          break;
        case OT_VIEW:
          expandType = OT_VIEW_SUBSTATICS;
          break;

        case OT_INDEX:
            // save dummy sub-item id for deletion
            idChildObj = GetFirstImmediateChild(lpDomData, idItem, 0);

            // Add sub-subitem INDEXSYNONYMS with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idItem,
                          lpRecord->extra,        // parent 0 : base
                          lpRecord->extra2,       // parent 1 : table
                          lpRecord->objName,      // parent 2 : index name
                          lpRecord->parentDbType,
                          lpRecord->ownerName,    // owner
                          lpRecord->szComplim,    // complimentary
                          lpRecord->tableOwner,   // schema for parent table
                          OT_STATIC_R_INDEXSYNONYM,
                          IDS_TREE_R_INDEXSYNONYM_STATIC, TRUE, FALSE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // delete dummy sub item
            TreeDeleteRecord(lpDomData, idChildObj);

            // say item is valid
            lpRecord->bSubValid = TRUE;

          return MergeResStatus(iretGlob, iret);
          break;

        default:
          expandType = 0;
      }
      if (!expandType)
        return MergeResStatus(iretGlob, RES_ERR);   // Unexpected type!

      iret = DOMUpdateDisplayData2(hnodestruct,
                                  expandType,
                                  resultLevel,
                                  aparentsResult,
                                  FALSE,                // bwithchildren
                                  iAction,
                                  idItem,
                                  hwndDOMDoc,
                                  forceRefreshMode);

      // set back top item
      SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

      return MergeResStatus(iretGlob, iret);
      break;


    //
    // EXPANSIONS LEADING TO STATIC SUB-ITEMS
    //
    case OT_DATABASE_SUBSTATICS:
    case OT_USER_SUBSTATICS:
    case OTR_GRANTS_SUBSTATICS:
    case OT_TABLE_SUBSTATICS:
    case OT_VIEW_SUBSTATICS:
    case OT_DBGRANTEES_SUBSTATICS:
    case OT_GROUP_SUBSTATICS:
    case OT_ROLE_SUBSTATICS:
    case OT_INDEX_SUBSTATICS:
    case OT_REPLICATOR_SUBSTATICS:
    case OT_ALARM_SUBSTATICS:
    case OT_SCHEMAUSER_SUBSTATICS:

    case OT_ICE_SUBSTATICS:
    case OT_ICE_SECURITY_SUBSTATICS:
    case OT_ICE_BUNIT_SUBSTATICS:
    case OT_ICE_SERVER_SUBSTATICS:

    case OT_INSTALL_SUBSTATICS:
    case OT_INSTALL_GRANTEES_SUBSTATICS:
    case OT_INSTALL_ALARMS_SUBSTATICS:

      // Here, we only add sub-static items to the current item,
      // copying the parenthood, owner and complimentary data
      // in each sub-static
      if (iAction != ACT_EXPANDITEM)
        return MergeResStatus(iretGlob, RES_ERR);   // FORBIDDEN OPERATION!

      // memorize top item
      idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                 0, 0L);

      idAnchorObj = idItem;
      lpRecord = (LPTREERECORD)SendMessage(lpDomData->hwndTreeLb,
                                           LM_GETITEMDATA,
                                           0, (LPARAM)idAnchorObj);

      // delete dummy sub-item
      idChildObj = GetFirstImmediateChild(lpDomData, idAnchorObj, 0);
      // FNN change 071795
      if (idChildObj && iobjecttype!=OT_REPLICATOR_SUBSTATICS)
        TreeDeleteRecord(lpDomData, idChildObj);

      // add sub-items according to the object type
      switch (iobjecttype) {
        case OT_DATABASE_SUBSTATICS:
          // Star management: create sub-branches according to Star feature
          databaseType = getint(lpRecord->szComplim+STEPSMALLOBJ);
          assert (databaseType == lpRecord->parentDbType);

          // Add sub-subitem TABLES with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_TABLE, IDS_TREE_TABLE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem VIEW with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_VIEW, IDS_TREE_VIEW_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Restricted features if gateway: plus schema, and that's all
          if (lpDomData->ingresVer==OIVERS_NOTOI) {
            char *VnodeName;
            NODESVRCLASSDATAMIN lpInfo;
            // Add sub-subitem SCHEMA with sub dummy collapsed
            // we put the database name in the extra data of the record
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_SCHEMA, IDS_TREE_SCHEMA_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            VnodeName = GetVirtNodeName (hnodestruct);
            lstrcpy((LPTSTR)lpInfo.NodeName,VnodeName);
            RemoveGWNameFromString(lpInfo.NodeName);
            GetGWClassNameFromString((LPUCHAR)VnodeName, lpInfo.ServerClass);
            if (INGRESII_IsDBEventsEnabled (&lpInfo))
           {
             // Add sub-subitem DBEVENT with sub dummy collapsed
             // we put the database name in the extra data of the record
             idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                           lpRecord->objName, NULL, NULL,
                           lpRecord->parentDbType,
                           NULL, NULL, NULL,
                           OT_STATIC_DBEVENT, IDS_TREE_DBEVENT_STATIC,
                           TRUE, TRUE);
             if (idChildObj==0)
               return MergeResStatus(iretGlob, RES_ERR);   // Fatal
			}
            break;
          }

          // Add sub-subitem PROCEDURE with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_PROCEDURE, IDS_TREE_PROCEDURE_STATIC,
                        TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // if DDB, add CDB branch and stop there
          if (databaseType == DBTYPE_DISTRIBUTED) {
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_R_CDB, IDS_TREE_R_CDB_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            break;
          }

            // Add sub-subitem SCHEMA with sub dummy collapsed
            // we put the database name in the extra data of the record
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_SCHEMA, IDS_TREE_SCHEMA_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          if (GetOIVers() >= OIVERS_30 ) {
           // Add sub-subitem SEQUENCE with sub dummy collapsed
           // we put the database name in the extra data of the record
           idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                         lpRecord->objName, NULL, NULL,
                         lpRecord->parentDbType,
                         NULL, NULL, NULL,
                         OT_STATIC_SEQUENCE, IDS_TREE_SEQUENCE_STATIC,
                         TRUE, TRUE);
           if (idChildObj==0)
             return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          }

          // Add sub-subitem SYNONYM with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_SYNONYM, IDS_TREE_SYNONYM_STATIC,
                        TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem DBGRANTEES with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_DBGRANTEES, IDS_TREE_DBGRANTEES_STATIC,
                        TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem DBEVENT with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBEVENT, IDS_TREE_DBEVENT_STATIC,
                          TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // Add sub-subitem REPLICATION with sub dummy collapsed
            // we put the database name in the extra data of the record
            // 19/9/95 : store db owner for call to GetReplicInstallStatus
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          lpRecord->ownerName, NULL, NULL,
                          OT_STATIC_REPLICATOR, IDS_TREE_REPLICATOR_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            //
            //  Beginning of DBALARMS full tree creation code
            //
            
            // Add sub-subitem DBALARMS on one level only
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM, IDS_TREE_DBALARM_STATIC,
                          FALSE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch CONNECT SUCCESS of DBALARM
            idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_CONN_SUCCESS,
                          IDS_TREE_DBALARM_CONN_SUCCESS_STATIC,
                          TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch CONNECT FAILURE of DBALARM
            idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_CONN_FAILURE,
                          IDS_TREE_DBALARM_CONN_FAILURE_STATIC,
                          TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch DISCONNECT SUCCESS of DBALARM
            idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_DISCONN_SUCCESS,
                          IDS_TREE_DBALARM_DISCONN_SUCCESS_STATIC,
                          TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch DISCONNECT FAILURE of DBALARM
            idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                          lpRecord->objName, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_DISCONN_FAILURE,
                          IDS_TREE_DBALARM_DISCONN_FAILURE_STATIC,
                          TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // set DBALARM sub-item in collapsed mode
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0, (LPARAM)idChildObj);
            
            //
            //  End of DBALARMS full tree creation code
            //

          break;

        case OT_USER_SUBSTATICS:
          // prepare buffers that will be used to fill the sub-items
          // we put the user's name in the extra data of each record,
          // plus ownership and complimentary data
          lstrcpy(buf,        lpRecord->objName);
          lstrcpy(bufOwner,   lpRecord->ownerName);
          lstrcpy(bufComplim, lpRecord->szComplim);

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the user's name in the extra data of the record
          // we delay the sub-items creation at expand time
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          //
          //  Beginning of REL SECURITY ALARMS full tree creation code
          //

          // Add sub-subitem REL SECURITY ALARMS on one level only
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SECURITY,
                        IDS_TREE_R_SECURITY_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          idChild3Obj = idChild2Obj;  // for minimum changes in the source

          // Sub branch SUCCESS SELECT of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_SEL_SUCC,
                        IDS_TREE_R_SEC_SEL_SUCC_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch FAILURE SELECT of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_SEL_FAIL,
                        IDS_TREE_R_SEC_SEL_FAIL_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch SUCCESS DELETE of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_DEL_SUCC,
                        IDS_TREE_R_SEC_DEL_SUCC_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch FAILURE DELETE of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_DEL_FAIL,
                        IDS_TREE_R_SEC_DEL_FAIL_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch SUCCESS INSERT of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_INS_SUCC,
                        IDS_TREE_R_SEC_INS_SUCC_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch FAILURE INSERT of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_INS_FAIL,
                        IDS_TREE_R_SEC_INS_FAIL_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch SUCCESS UPDATE of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_UPD_SUCC,
                        IDS_TREE_R_SEC_UPD_SUCC_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Sub branch FAILURE UPDATE of SECURITY
          idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_SEC_UPD_FAIL,
                        IDS_TREE_R_SEC_UPD_FAIL_STATIC, TRUE, TRUE);
          if (idChild4Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set SECURITY sub-item in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          //
          //  End of REL SECURITY full tree creation code
          //

          // Stop here if user name is "(public)"
          if (x_strcmp(buf, lppublicdispstring()) == 0) {
            // full collapsed mode force (SECURITY sub-item)
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0, (LPARAM)idChild2Obj);
            break;
          }

          // Add sub-subitem R_USERSCHEMA with sub dummy collapsed
          // we put the user's name in the extra data of the record
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_USERSCHEMA,
                        IDS_TREE_R_USERSCHEMA_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem R_USERGROUP with sub dummy collapsed
          // we put the user's name in the extra data of the record
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_USERGROUP,
                        IDS_TREE_R_USERGROUP_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OTR_GRANTS_SUBSTATICS:
          // prepare buffers that will be used to fill the sub-items
          // we put the user/group/role name in the extra data
          // of each record, plus ownership and complimentary data.
          lstrcpy(buf,        lpRecord->extra);
          lstrcpy(bufOwner,   lpRecord->ownerName);
          lstrcpy(bufComplim, lpRecord->szComplim);

          //
          //  Beginning of GRANTS full tree creation code
          //

          // branch DBGRANT
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBGRANT,
                        IDS_TREE_R_DBGRANT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch ACCESSY of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBGRANT_ACCESY,
                        IDS_TREE_R_DBGRANT_ACCESY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch ACCESSN of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_ACCESN,
                          IDS_TREE_R_DBGRANT_ACCESN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CREPRY of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_CREPRY,
                          IDS_TREE_R_DBGRANT_CREPRY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
          // sub branch CREPRN of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_CREPRN,
                          IDS_TREE_R_DBGRANT_CREPRN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CRETBY of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBGRANT_CRETBY,
                        IDS_TREE_R_DBGRANT_CRETBY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CRETBN of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_CRETBN,
                          IDS_TREE_R_DBGRANT_CRETBN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch DBADMY of DBGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBGRANT_DBADMY,
                        IDS_TREE_R_DBGRANT_DBADMY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch DBADMN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_DBADMN,
                          IDS_TREE_R_DBGRANT_DBADMN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch LKMODY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_LKMODY,
                          IDS_TREE_R_DBGRANT_LKMODY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch LKMODN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_LKMODN,
                          IDS_TREE_R_DBGRANT_LKMODN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYIOY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYIOY,
                          IDS_TREE_R_DBGRANT_QRYIOY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYION of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYION,
                          IDS_TREE_R_DBGRANT_QRYION_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYRWY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYRWY,
                          IDS_TREE_R_DBGRANT_QRYRWY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYRWN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYRWN,
                          IDS_TREE_R_DBGRANT_QRYRWN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch UPDSCY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_UPDSCY,
                          IDS_TREE_R_DBGRANT_UPDSCY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch UPDSCN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_UPDSCN,
                          IDS_TREE_R_DBGRANT_UPDSCN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SELSCY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_SELSCY,
                          IDS_TREE_R_DBGRANT_SELSCY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SELSCN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_SELSCN,
                          IDS_TREE_R_DBGRANT_SELSCN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CNCTLY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_CNCTLY,
                          IDS_TREE_R_DBGRANT_CNCTLY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CNCTLN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_CNCTLN,
                          IDS_TREE_R_DBGRANT_CNCTLN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch IDLTLY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_IDLTLY,
                          IDS_TREE_R_DBGRANT_IDLTLY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch IDLTLN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_IDLTLN,
                          IDS_TREE_R_DBGRANT_IDLTLN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SESPRY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_SESPRY,
                          IDS_TREE_R_DBGRANT_SESPRY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SESPRN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_SESPRN,
                          IDS_TREE_R_DBGRANT_SESPRN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch TBLSTY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_TBLSTY,
                          IDS_TREE_R_DBGRANT_TBLSTY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch TBLSTN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_TBLSTN,
                          IDS_TREE_R_DBGRANT_TBLSTN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch QRYCPY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYCPY,
                          IDS_TREE_R_DBGRANT_QRYCPY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYCPN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYCPN,
                          IDS_TREE_R_DBGRANT_QRYCPN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            // sub branch QRYPGY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYPGY,
                          IDS_TREE_R_DBGRANT_QRYPGY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYPGN of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYPGN,
                          IDS_TREE_R_DBGRANT_QRYPGN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            // sub branch QRYCOY of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYCOY,
                          IDS_TREE_R_DBGRANT_QRYCOY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYCON of DBGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_DBGRANT_QRYCON,
                          IDS_TREE_R_DBGRANT_QRYCON_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            if ( GetOIVers() >= OIVERS_30 )
            {
              // sub branch CRESEQY of DBGRANT
              idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                            buf, NULL, NULL,
                            lpRecord->parentDbType,
                            bufOwner, bufComplim, NULL,
                            OT_STATIC_R_DBGRANT_CRESEQY,
                           IDS_TREE_R_DBGRANT_CRESEQY_STATIC, TRUE, TRUE);
              if (idChild3Obj==0)
                return MergeResStatus(iretGlob, RES_ERR);   // Fatal

              // sub branch CRESEQN of DBGRANT
              idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                            buf, NULL, NULL,
                            lpRecord->parentDbType,
                            bufOwner, bufComplim, NULL,
                            OT_STATIC_R_DBGRANT_CRESEQN,
                            IDS_TREE_R_DBGRANT_CRESEQN_STATIC, TRUE, TRUE);
              if (idChild3Obj==0)
                return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            }

          // set DBGRANT branch in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          // branch TABLEGRANT
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT,
                        IDS_TREE_R_TABLEGRANT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch SEL of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_SEL,
                        IDS_TREE_R_TABLEGRANT_SEL_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch INS of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_INS,
                        IDS_TREE_R_TABLEGRANT_INS_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch UPD of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_UPD,
                        IDS_TREE_R_TABLEGRANT_UPD_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch DEL of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_DEL,
                        IDS_TREE_R_TABLEGRANT_DEL_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch REF of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_REF,
                        IDS_TREE_R_TABLEGRANT_REF_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CPI of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_CPI,
                        IDS_TREE_R_TABLEGRANT_CPI_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CPF of TABLEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_TABLEGRANT_CPF,
                        IDS_TREE_R_TABLEGRANT_CPF_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

//          // sub branch ALL of TABLEGRANT
//          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
//                        buf, NULL, NULL,
//                        lpRecord->parentDbType,
//                        bufOwner, bufComplim, NULL,
//                        OT_STATIC_R_TABLEGRANT_ALL,
//                        IDS_TREE_R_TABLEGRANT_ALL_STATIC, TRUE, TRUE);
//          if (idChild3Obj==0)
//            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set TABLEGRANT branch in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          // branch VIEWGRANT
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_VIEWGRANT,
                        IDS_TREE_R_VIEWGRANT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch SEL of VIEWGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_VIEWGRANT_SEL,
                        IDS_TREE_R_VIEWGRANT_SEL_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch INS of VIEWGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_VIEWGRANT_INS,
                        IDS_TREE_R_VIEWGRANT_INS_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch UPD of VIEWGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_VIEWGRANT_UPD,
                        IDS_TREE_R_VIEWGRANT_UPD_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch DEL of VIEWGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_VIEWGRANT_DEL,
                        IDS_TREE_R_VIEWGRANT_DEL_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set VIEWGRANT branch in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          // branch DBEGRANT
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBEGRANT,
                        IDS_TREE_R_DBEGRANT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch RAISE of DBEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBEGRANT_RAISE,
                        IDS_TREE_R_DBEGRANT_RAISE_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch REGISTER of DBEGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_DBEGRANT_REGISTER,
                        IDS_TREE_R_DBEGRANT_REGISTER_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set DBEGRANT branch in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          // branch PROCGRANT
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_PROCGRANT,
                        IDS_TREE_R_PROCGRANT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch EXEC of PROCGRANT
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        buf, NULL, NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, NULL,
                        OT_STATIC_R_PROCGRANT_EXEC,
                        IDS_TREE_R_PROCGRANT_EXEC_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set PROCGRANT branch in collapsed mode
          // don't use fast collapse in case no rolegrant branch
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0, (LPARAM)idChild2Obj);

          if ( GetOIVers() >= OIVERS_30 ) {
            // branch SEQUENCE GRANT
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_SEQGRANT,
                          IDS_TREE_R_SEQGRANT_STATIC, FALSE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch EXEC of PROCGRANT
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_SEQGRANT_NEXT,
                          IDS_TREE_R_SEQGRANT_NEXT_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // set SEQGRANT branch in collapsed mode
            // don't use fast collapse in case no rolegrant branch
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0, (LPARAM)idChild2Obj);
          }

          // branch ROLEGRANT - only for users
          iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                                buf,            // object name
                                bufOwner,
                                OT_USER,
                                0,
                                NULL,
                                TRUE,           // bwithsystem
                                &resultType,
                                &resultLevel,
                                aparentsResult, // array of result parent strings
                                buf2,           // result object name
                                bufOwner,       // result owner
                                bufComplim);    // result complimentary data
          if (iret == RES_ENDOFDATA)
            bUser = FALSE;
          else
            bUser =TRUE;
          if (bUser) {
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_R_ROLEGRANT,
                          IDS_TREE_R_ROLEGRANT_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // no sub-branches

            // set ROLEGRANT branch in collapsed mode
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0, (LPARAM)idChild2Obj);
          }

          //
          //  End of GRANTS full tree creation code
          //

          break;

        case OT_TABLE_SUBSTATICS:

          // Star management: create sub-branches according to Star feature
          objType = getint(lpRecord->szComplim + STEPSMALLOBJ);

          // if star native object, no sub branches at all
          if (objType == OBJTYPE_STARNATIVE)
            break;

          // prepare buffers that will be used to fill the sub-items :
          // We put table name in the first extra data of the records,
          // and the base name in the second extra data of the records,
          // plus the ownership and complimentary info
          lstrcpy(aparentsResult[0], lpRecord->extra);
          lstrcpy(aparentsResult[1], lpRecord->objName);
          aparentsResult[2][0] = '\0';
          lstrcpy(bufOwner, lpRecord->ownerName);
          lstrcpy(bufComplim, lpRecord->szComplim);

          // special : for indexes, integrities and rules,
          // put immediately schema.objName as parent of level 1
          StringWithOwner(lpRecord->objName, bufOwner, buf2);

          // Restricted features if gateway
          if (lpDomData->ingresVer==OIVERS_NOTOI) {
            // Add sub-subitem INDEX with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf2,       // schema - instead of aparentsResult[1]
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_INDEX,
                          IDS_TREE_INDEX_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            break;
          }     // End of Restricted features if gateway

          // if star registered as link object, only Add sub-subitem INDEX with sub dummy collapsed
          if (objType == OBJTYPE_STARLINK) {
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf2,       // schema - instead of aparentsResult[1]
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_INDEX,
                          IDS_TREE_INDEX_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            break;
          }

          // Add sub-subitems with sub dummies collapsed
            // Add sub-subitem INTEGRITY with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf2,       // schema - instead of aparentsResult[1]
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_INTEGRITY,
                          IDS_TREE_INTEGRITY_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem RULE with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf2,       // schema - instead of aparentsResult[1]
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_RULE,
                          IDS_TREE_RULE_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            //
            //  Beginning of SECURITY full tree creation code
            //
            
            // Add sub-subitem SECURITY ALARMS on one level only
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SECURITY,
                          IDS_TREE_SECURITY_STATIC, FALSE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            idChild3Obj = idChild2Obj;  // for minimum changes in the source
            
            // Sub branch SUCCESS SELECT of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_SEL_SUCC,
                          IDS_TREE_SEC_SEL_SUCC_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch FAILURE SELECT of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table     
                          OT_STATIC_SEC_SEL_FAIL,
                          IDS_TREE_SEC_SEL_FAIL_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch SUCCESS DELETE of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_DEL_SUCC,
                          IDS_TREE_SEC_DEL_SUCC_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch FAILURE DELETE of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_DEL_FAIL,
                          IDS_TREE_SEC_DEL_FAIL_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch SUCCESS INSERT of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_INS_SUCC,
                          IDS_TREE_SEC_INS_SUCC_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch FAILURE INSERT of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_INS_FAIL,
                          IDS_TREE_SEC_INS_FAIL_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch SUCCESS UPDATE of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_UPD_SUCC,
                          IDS_TREE_SEC_UPD_SUCC_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch FAILURE UPDATE of SECURITY
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_SEC_UPD_FAIL,
                          IDS_TREE_SEC_UPD_FAIL_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // set SECURITY sub-item in fast collapsed mode
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0xdcba, (LPARAM)idChild2Obj);
            
            //
            //  End of SECURITY full tree creation code
            //

          // Add sub-subitem INDEX with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        buf2,       // schema - instead of aparentsResult[1]
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_INDEX,
                        IDS_TREE_INDEX_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // Add sub-subitem TABLELOCATION with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_TABLELOCATION,
                          IDS_TREE_TABLELOCATION_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          //
          //  Beginning of TABLEGRANTEES full tree creation code
          //

          // Add sub-subitem TABLEGRANTEES with several sub-branches,
          // each with a sub dummy
          bSubDummy = TRUE;

          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_TABLEGRANTEES,
                        IDS_TREE_TABLEGRANTEES_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_SEL_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_TABLEGRANT_SEL_USER,
                        IDS_TREE_TABLEGRANT_SEL_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_INS_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_TABLEGRANT_INS_USER,
                        IDS_TREE_TABLEGRANT_INS_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_UPD_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_TABLEGRANT_UPD_USER,
                        IDS_TREE_TABLEGRANT_UPD_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_DEL_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_TABLEGRANT_DEL_USER,
                        IDS_TREE_TABLEGRANT_DEL_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_REF_USER
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_TABLEGRANT_REF_USER,
                          IDS_TREE_TABLEGRANT_REF_USER_STATIC, bSubDummy, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_CPI_USER
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_TABLEGRANT_CPI_USER,
                          IDS_TREE_TABLEGRANT_CPI_USER_STATIC, bSubDummy, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch TABLEGRANT_CPF_USER
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_TABLEGRANT_CPF_USER,
                          IDS_TREE_TABLEGRANT_CPF_USER_STATIC, bSubDummy, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

//          // sub branch TABLEGRANT_ALL_USER
//          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
//                        aparentsResult[0],
//                        aparentsResult[1],
//                        aparentsResult[2],
//                        lpRecord->parentDbType,
//                        bufOwner, bufComplim,
//                        bufOwner,               // table schema
//                        OT_STATIC_TABLEGRANT_ALL_USER,
//                        IDS_TREE_TABLEGRANT_ALL_USER_STATIC, bSubDummy, TRUE);
//          if (idChild3Obj==0)
//            return MergeResStatus(iretGlob, RES_ERR);   // Fatal


          // set TABLEGRANTEES sub-item in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          //
          //  End of TABLEGRANTEES full tree creation code
          //

          // Add sub-subitem TABLESYNONYMS with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_R_TABLESYNONYM,
                        IDS_TREE_R_TABLESYNONYM_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem TABLEVIEWS ( related views)
          // with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        aparentsResult[2],
                        lpRecord->parentDbType,
                        bufOwner, bufComplim,
                        bufOwner,               // table schema
                        OT_STATIC_R_TABLEVIEW,
                        IDS_TREE_R_TABLEVIEW_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // Add sub-subitem CDDS ON TABLE with sub dummy collapsed,
            // with full collapse
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          aparentsResult[1],
                          aparentsResult[2],
                          lpRecord->parentDbType,
                          bufOwner, bufComplim,
                          bufOwner,               // table schema
                          OT_STATIC_R_REPLIC_TABLE_CDDS,
                          IDS_TREE_R_REPLIC_TABLE_CDDS_STATIC, TRUE, FALSE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;
        case OT_SEQUENCE:

          // prepare buffers that will be used to fill the sub-items :
          // We put base name in the first extra data of the record,
          // and the view name in the second extra data of the record
          // plus the ownership and complimentary info
          lstrcpy(aparentsResult[0], lpRecord->extra);
          lstrcpy(buf,               lpRecord->objName);
          lstrcpy(bufOwner,          lpRecord->ownerName);
          lstrcpy(bufComplim,        lpRecord->szComplim);

          if ( GetOIVers() >= OIVERS_30 )
          {

            // Add sub-subitem SEQUENCES with sub dummy collapsed
            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf,
                          NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, bufOwner,
                          OT_STATIC_SEQUENCE,
                          IDS_TREE_SEQUENCE_STATIC, TRUE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal


            // Add sub-subitem SEQUENCE Grantees (Next for Sequences),
            // each with a sub dummy
            bSubDummy = TRUE;

            idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                          aparentsResult[0],
                          buf,
                          NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, bufOwner,
                          OT_STATIC_SEQGRANT_NEXT_USER,
                          IDS_TREE_SEQUENCE_GRANT_STATIC, FALSE, TRUE);
            if (idChild2Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // set Sequences Grantees sub-item in fast collapsed mode
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0xdcba, (LPARAM)idChild2Obj);
          }
          break;

        case OT_VIEW_SUBSTATICS:

          // Star management: create sub-branches according to Star feature
          objType = getint(lpRecord->szComplim + 4 + STEPSMALLOBJ);

          // if star Link object, no sub branches at all
          if (objType == OBJTYPE_STARLINK)
            break;

          // prepare buffers that will be used to fill the sub-items :
          // We put base name in the first extra data of the record,
          // and the view name in the second extra data of the record
          // plus the ownership and complimentary info
          lstrcpy(aparentsResult[0], lpRecord->extra);
          lstrcpy(buf,               lpRecord->objName);
          lstrcpy(bufOwner,          lpRecord->ownerName);
          lstrcpy(bufComplim,        lpRecord->szComplim);

          // Add sub-subitem VIEWTABLE with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWTABLE,
                        IDS_TREE_VIEWTABLE_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Restricted features if gateway
          if (lpDomData->ingresVer==OIVERS_NOTOI)
            break;

          // if star Native object, only view components branch (Star Link object few lines upper)
          if (objType == OBJTYPE_STARNATIVE)
            break;

          // if View was created with QUEL language, only view components branch
          bViewSqlType = getint(lpRecord->szComplim + 4 + (STEPSMALLOBJ*2));
          if (!bViewSqlType)
            break;

          //
          //  Beginning of VIEWGRANTEES full tree creation code
          //


          // Add sub-subitem VIEWGRANTEES with 4 sub-branches,
          // each with a sub dummy
          bSubDummy = TRUE;

          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWGRANTEES,
                        IDS_TREE_VIEWGRANTEES_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch VIEWGRANT_SEL_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWGRANT_SEL_USER,
                        IDS_TREE_VIEWGRANT_SEL_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch VIEWGRANT_INS_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWGRANT_INS_USER,
                        IDS_TREE_VIEWGRANT_INS_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch VIEWGRANT_UPD_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWGRANT_UPD_USER,
                        IDS_TREE_VIEWGRANT_UPD_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch VIEWGRANT_DEL_USER
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_VIEWGRANT_DEL_USER,
                        IDS_TREE_VIEWGRANT_DEL_USER_STATIC, bSubDummy, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // set VIEWGRANTEES sub-item in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          //
          //  End of VIEWGRANTEES full tree creation code
          //

          // Add sub-subitem VIEWSYNONYMS with sub dummy collapsed
          // use full collapse
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        buf,
                        NULL,
                        lpRecord->parentDbType,
                        bufOwner, bufComplim, bufOwner,
                        OT_STATIC_R_VIEWSYNONYM,
                        IDS_TREE_R_VIEWSYNONYM_STATIC, TRUE, FALSE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          break;

        case OT_DBGRANTEES_SUBSTATICS:
        case OT_INSTALL_GRANTEES_SUBSTATICS:  // SPECIAL : No database name...
          // prepare buffers that will be used to fill the sub-items
          // we put the database name in the extra data of each record,
          // plus ownership and complimentary data.
          lstrcpy(buf,        lpRecord->extra);
          lstrcpy(bufOwner,   lpRecord->ownerName);
          lstrcpy(bufComplim, lpRecord->szComplim);

          idChild2Obj = idAnchorObj;  // minimize changes in code

          // sub branch ACCESSY of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_ACCESY,
                          IDS_TREE_DBGRANTEES_ACCESY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch ACCESSN of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_ACCESN,
                          IDS_TREE_DBGRANTEES_ACCESN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CREPRY of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CREPRY,
                          IDS_TREE_DBGRANTEES_CREPRY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
          // sub branch CREPRN of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CREPRN,
                          IDS_TREE_DBGRANTEES_CREPRN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CRETBY of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CRETBY,
                          IDS_TREE_DBGRANTEES_CRETBY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // sub branch CRETBN of DBGRANTEES
          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CRETBN,
                          IDS_TREE_DBGRANTEES_CRETBN_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_DBADMY,
                          IDS_TREE_DBGRANTEES_DBADMY_STATIC, TRUE, TRUE);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch DBADMN of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_DBADMN,
                          IDS_TREE_DBGRANTEES_DBADMN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch LKMODY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_LKMODY,
                          IDS_TREE_DBGRANTEES_LKMODY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch LKMODN of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_LKMODN,
                          IDS_TREE_DBGRANTEES_LKMODN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYIOY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYIOY,
                          IDS_TREE_DBGRANTEES_QRYIOY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYION of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYION,
                          IDS_TREE_DBGRANTEES_QRYION_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYRWY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYRWY,
                          IDS_TREE_DBGRANTEES_QRYRWY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYRWN of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYRWN,
                          IDS_TREE_DBGRANTEES_QRYRWN_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch UPDSCY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_UPDSCY,
                          IDS_TREE_DBGRANTEES_UPDSCY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch UPDSCN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_UPDSCN,
                          IDS_TREE_DBGRANTEES_UPDSCN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SELSCY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_SELSCY,
                          IDS_TREE_DBGRANTEES_SELSCY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SELSCN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_SELSCN,
                          IDS_TREE_DBGRANTEES_SELSCN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CNCTLY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CNCTLY,
                          IDS_TREE_DBGRANTEES_CNCTLY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch CNCTLN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_CNCTLN,
                          IDS_TREE_DBGRANTEES_CNCTLN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch IDLTLY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_IDLTLY,
                          IDS_TREE_DBGRANTEES_IDLTLY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch IDLTLN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_IDLTLN,
                          IDS_TREE_DBGRANTEES_IDLTLN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SESPRY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_SESPRY,
                          IDS_TREE_DBGRANTEES_SESPRY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch SESPRN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_SESPRN,
                          IDS_TREE_DBGRANTEES_SESPRN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch TBLSTY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_TBLSTY,
                          IDS_TREE_DBGRANTEES_TBLSTY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch TBLSTN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_TBLSTN,
                          IDS_TREE_DBGRANTEES_TBLSTN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch QRYCPY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYCPY,
                          IDS_TREE_DBGRANTEES_QRYCPY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYCPN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYCPN,
                          IDS_TREE_DBGRANTEES_QRYCPN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch QRYPGY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYPGY,
                          IDS_TREE_DBGRANTEES_QRYPGY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYPGN of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYPGN,
                          IDS_TREE_DBGRANTEES_QRYPGN_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // sub branch QRYCOY of DBGRANTEES
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYCOY,
                          IDS_TREE_DBGRANTEES_QRYCOY_STATIC, TRUE, TRUE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // sub branch QRYCON of DBGRANTEES
            // use full collapse
            idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                          buf, NULL, NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, NULL,
                          OT_STATIC_DBGRANTEES_QRYCON,
                          IDS_TREE_DBGRANTEES_QRYCON_STATIC, TRUE, FALSE);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            if ( GetOIVers() >= OIVERS_30 )
            {
              // sub branch CRESEQY of DBGRANTEES
              idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                            buf, NULL, NULL,
                            lpRecord->parentDbType,
                            bufOwner, bufComplim, NULL,
                            OT_STATIC_DBGRANTEES_CRSEQY,
                            IDS_TREE_DBGRANTEES_CRSEQY_STATIC, TRUE, TRUE);
              if (idChild3Obj==0)
                return MergeResStatus(iretGlob, RES_ERR);   // Fatal
              // sub branch CRESEQN of DBGRANTEES
              // use full collapse
              idChild3Obj = AddStaticSubItem(lpDomData, idChild2Obj,
                            buf, NULL, NULL,
                            lpRecord->parentDbType,
                            bufOwner, bufComplim, NULL,
                            OT_STATIC_DBGRANTEES_CRSEQN,
                            IDS_TREE_DBGRANTEES_CRSEQN_STATIC, TRUE, FALSE);
            }
          break;

        case OT_GROUP_SUBSTATICS:
          // Add sub-subitem GROUPUSERS with sub dummy collapsed
          // we put the group name in the extra data of the record
          // along with owner name and complim data
          idChildObj =  AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName, lpRecord->szComplim, NULL,
                        OT_STATIC_GROUPUSER,
                        IDS_TREE_GROUPUSER_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the group name in the extra data of the record
          // along with owner name and complim data
          // we delay the sub-items creation at expand time
          // use full collapse
          idChildObj =  AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName, lpRecord->szComplim, NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, FALSE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ROLE_SUBSTATICS:
          // Add sub-subitem R_GRANTS with sub dummy collapsed
          // we put the role name in the extra data of the record
          // along with owner name and complim data
          // we delay the sub-items creation at expand time
          idChildObj =  AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName, lpRecord->szComplim, NULL,
                        OT_STATIC_R_GRANT,
                        IDS_TREE_R_GRANT_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem ROLEGRANT_USER with sub dummy collapsed
          // we put the role name in the extra data of the record
          // along with owner name and complim data
          // Use full collapse
          idChildObj =  AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        lpRecord->parentDbType,
                        lpRecord->ownerName, lpRecord->szComplim, NULL,
                        OT_STATIC_ROLEGRANT_USER,
                        IDS_TREE_ROLEGRANT_USER_STATIC, TRUE, FALSE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_INDEX_SUBSTATICS:
            // Add sub-subitem INDEXSYNONYMS with sub dummy collapsed
            // we put the base, table and index name
            // in the extra datas of the record
            // along with owner name and complim data
            // we delay the sub-items creation at expand time
            // use full collapse
            idChildObj =  AddStaticSubItem(lpDomData, idAnchorObj,
                          lpRecord->extra,
                          lpRecord->extra2,
                          lpRecord->objName,
                          lpRecord->parentDbType,
                          lpRecord->ownerName, lpRecord->szComplim,
                          lpRecord->tableOwner,
                          OT_STATIC_R_INDEXSYNONYM,
                          IDS_TREE_R_INDEXSYNONYM_STATIC, TRUE, FALSE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_REPLICATOR_SUBSTATICS:


          // added FNN 071795
          TreeDeleteAllChildren(lpDomData, idAnchorObj);

          // pre-special case : replic_v11 not fully installed
          // (repcat used but repmgr not used)
          nReplVer=GetReplicInstallStatus(hnodestruct, lpRecord->extra,
                                           lpRecord->ownerName);
          if (nReplVer == REPLIC_V10  ||
              nReplVer == REPLIC_V105 ||
              nReplVer == REPLIC_V11  ) {
             int ret;
             ret = ReplicatorLocalDbInstalled(hnodestruct,
                                              lpRecord->extra,
                                              nReplVer );
             switch (ret) {
               case RES_ERR:
                 if(FillReplicLocalDBInformation(hnodestruct,
                                                 lpRecord,
                                                 hwndDOMDoc,
                                                 nReplVer )==FALSE) {
                    // rebuild dummy sub-item and collapse it
                    idChildObj = AddDummySubItem(lpDomData, idAnchorObj);
                    PostMessage(lpDomData->hwndTreeLb, LM_COLLAPSEBRANCH,
                                0, (LPARAM)idAnchorObj);
                    lpRecord->bSubValid = FALSE;
                    return RES_ERR; // otherwise caller in tree.c (response to LM_NOTIFY_EXPAND) sets lpRecord->bSubValid MergeResStatus(iretGlob, RES_SUCCESS);
                 }
                 break;
               case RES_NOGRANT:
               case RES_SQLNOEXEC:
                 // replace dummy sub-item with error indication item
                 AddProblemSubItem(lpDomData, idAnchorObj, ret, OT_REPLIC_CONNECTION, lpRecord);
                 return MergeResStatus(iretGlob, RES_NOGRANT);
             }
          }


          // First of all : test whether replicator is installed
          // if not, only create sub item saying it is not,
          // and don't say sub branch is valid
          if (GetReplicInstallStatus(hnodestruct, lpRecord->extra,
                                                   lpRecord->ownerName)
              ==REPLIC_NOINSTALL) {
            LoadString(hResource, IDS_TREE_REPLICATOR_NONE,
                              bufRs, sizeof(bufRs));

            // Add sub-line saying replicator is not installed
            lpRecord = AllocAndFillRecord(OT_REPLICATOR, TRUE,
                                          lpRecord->extra,
                                          NULL,
                                          NULL,
                                          lpRecord->parentDbType,
                                          bufRs, bufOwner, bufComplim, NULL);
            if (!lpRecord)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            idChildObj = TreeAddRecord(lpDomData, bufRs, idAnchorObj,
                                      0, 0, lpRecord);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // Terminal branch -
            // No dummy sub-sub-item for this kind of sub items



            // set back top item
            SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

            return MergeResStatus(iretGlob, RES_SUCCESS);
            break;
          }

          nReplVer=GetReplicInstallStatus(hnodestruct, lpRecord->extra,
                                           lpRecord->ownerName);

          // Emb 21/3/96 : change caption of replicator line
          // according to the version
          // Plus 5/5/97 : special caption for open ingres 2.0
          x_strcpy(bufRs, ResourceString(IDS_TREE_REPLICATOR_STATIC));
          if (GetOIVers() == OIVERS_20 )
            x_strcat(bufRs, " 2.0");
		  else if (GetOIVers() < OIVERS_20) {
		    switch ( nReplVer ) {
              case REPLIC_V10:
                x_strcat(bufRs, " 1.0");
                break;
              case REPLIC_V105:
                x_strcat(bufRs, " 1.05");
                break;
              case REPLIC_V11:
                x_strcat(bufRs, " 1.1");
                break;
            }
          }
          setString.lpString  = bufRs;
          setString.ulRecID   = idAnchorObj;
          SendMessage(lpDomData->hwndTreeLb, LM_SETSTRING, 0,
                          (LPARAM) (LPSETSTRING)&setString );

          // Add sub-subitem CONNECTION with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->extra, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_REPLIC_CONNECTION,
                        IDS_TREE_REPLIC_CONNECTION_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem CDDS with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->extra, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_REPLIC_CDDS,
                        IDS_TREE_REPLIC_CDDS_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem MAILUSER with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->extra, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_REPLIC_MAILUSER,
                        IDS_TREE_REPLIC_MAILUSER_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem REGTABLE with sub dummy collapsed
          // we put the database name in the extra data of the record
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->extra, NULL, NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_REPLIC_REGTABLE,
                        IDS_TREE_REPLIC_REGTABLE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ALARM_SUBSTATICS:
          // prepare variables - serious stuff!
          lstrcpy(aparentsResult[0], lpRecord->extra);    // parent database
          lstrcpy(aparentsResult[1], lpRecord->extra2);   // parent table
          lstrcpy(buf,               lpRecord->objName);    // alarmee name
          lstrcpy(bufOwner,          lpRecord->ownerName);  // schema.dbevent
          if (bufOwner[0]) {
            // build dbevent name and dbevent owner from bufOwner :
            // dbevent name will be in buf2
            // dbevent owner will be in bufOwner

            // anchor aparentsResult2 on buffers
            aparentsResult2[0] = bufParResult02;
            aparentsResult2[1] = bufParResult12;
            aparentsResult2[2] = bufParResult22;
          
            iret = DOMGetObject(hnodestruct,
                                bufOwner,
                                "",
                                OT_DBEVENT,
                                1,                // level
                                aparentsResult,   // aparents
                                bwithsystem,
                                &resultType,
                                &resultLevel,
                                aparentsResult2,  // array of result parent strings
                                buf2,             // result object name
                                buf3,             // result owner
                                buf4);            // result complimentary data
            if (iret != RES_SUCCESS)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            x_strcpy(bufOwner, buf3);
          }
          else {
            // dbevent name will be in buf2, empty
            // dbevent owner will be in bufOwner, empty
            buf2[0] = '\0';
          }

          // Add sub-subitem STATIC ALARMEE with no sub-dummy
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_ALARMEE,
                        IDS_TREE_ALARMEE_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add a sub-item to the static : the alarmee object
          lpRecord2 = AllocAndFillRecord(OT_ALARMEE, FALSE,
                                    aparentsResult[0],
                                    aparentsResult[1],
                                    NULL,
                                    lpRecord->parentDbType,
                                    buf, NULL, NULL,
                                    NULL);
          if (!lpRecord2)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          idChild3Obj = TreeAddRecord(lpDomData,
                                      buf,
                                      idChild2Obj,
                                      0, 0, // 20-Jan-2000 (noifr01) single item there, no sort: this allows to put 0 and get rid of the warning
                                      lpRecord2);
          if (idChild3Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // add a dummy sub-sub-item to the synonymed object tree line
          AddDummySubItem(lpDomData, idChild3Obj);

          // Set the sub-sub-sub item in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild3Obj);

          // Set the sub-sub item in fast collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0xdcba, (LPARAM)idChild2Obj);

          // Add sub-subitem STATIC ALARM_EVENT with no sub-dummy
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_ALARM_EVENT,
                        IDS_TREE_ALARM_EVENT_STATIC, FALSE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add a sub-item to the static : the alarm event object
          // plus sub-items raise & register users granted on the dbevent
          // or the string < No Launched Dbevent > if none
          if (buf2[0]) {
            lpRecord2 = AllocAndFillRecord(OT_S_ALARM_EVENT, TRUE,
                                      aparentsResult[0],
                                      aparentsResult[1],
                                      NULL,
                                      lpRecord->parentDbType,
                                      buf2, bufOwner, NULL,
                                      NULL);
            if (!lpRecord2)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            idChild3Obj = TreeAddRecord(lpDomData,
                                        buf2,
                                        idChild2Obj,
                                         0, 0, // 20-Jan-2000 (noifr01) single item there, no sort: this allows to put 0 and get rid of the warning
                                        lpRecord2);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // add the sub-branches DBEGRANT_RAISE_USER
            // and DBEGRANT_REGTR_USER
            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          buf2,
                          NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, bufOwner,
                          OT_STATIC_DBEGRANT_RAISE_USER,
                          IDS_TREE_DBEGRANT_RAISE_USER_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            idChild4Obj = AddStaticSubItem(lpDomData, idChild3Obj,
                          aparentsResult[0],
                          buf,
                          NULL,
                          lpRecord->parentDbType,
                          bufOwner, bufComplim, bufOwner,
                          OT_STATIC_DBEGRANT_REGTR_USER,
                          IDS_TREE_DBEGRANT_REGTR_USER_STATIC, TRUE, TRUE);
            if (idChild4Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // Set the sub item in fast collapsed mode
            SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                        0xdcba, (LPARAM)idChild3Obj);
          }
          else {
            // < No Launched Dbevent >, with no sub-item
            LoadString(hResource, IDS_TREE_ALARM_EVENT_NONE, buf2, sizeof(buf2));
            lpRecord2 = AllocAndFillRecord(OT_S_ALARM_EVENT, FALSE,
                                      aparentsResult[0],
                                      aparentsResult[1],
                                      NULL,
                                      lpRecord->parentDbType,
                                      buf2, bufOwner, NULL,
                                      NULL);
            if (!lpRecord2)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            idChild3Obj = TreeAddRecord(lpDomData,
                                        buf2,
                                        idChild2Obj,
                                        0, 0, // 20-Jan-2000 (noifr01) single item there, no sort: this allows to put 0 and get rid of the warning
                                        lpRecord2);
            if (idChild3Obj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          }

          // Set the sub-sub item in full collapsed mode
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                      0, (LPARAM)idChild2Obj);

          break;

        case OT_SCHEMAUSER_SUBSTATICS:
          lstrcpy(aparentsResult[0], lpRecord->extra);    // dbname
          lstrcpy(aparentsResult[1], lpRecord->objName);  // schema name

          // Add sub-subitem TABLE with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_SCHEMAUSER_TABLE,
                        IDS_TREE_SCHEMAUSER_TABLE_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add sub-subitem VIEW with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_SCHEMAUSER_VIEW,
                        IDS_TREE_SCHEMAUSER_VIEW_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Restricted feature if gateway: only tables and views
          if (lpDomData->ingresVer==OIVERS_NOTOI)
            break;

          // Add sub-subitem PROCEDURE with sub dummy collapsed
          idChild2Obj = AddStaticSubItem(lpDomData, idAnchorObj,
                        aparentsResult[0],
                        aparentsResult[1],
                        NULL,
                        lpRecord->parentDbType,
                        NULL, NULL, NULL,
                        OT_STATIC_SCHEMAUSER_PROCEDURE,
                        IDS_TREE_SCHEMAUSER_PROCEDURE_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ICE_SUBSTATICS:
          {
          char szVnodeName[BUFSIZE];
          TreeDeleteAllChildren(lpDomData, idAnchorObj);

          lstrcpy(szVnodeName,GetVirtNodeName (hnodestruct));
          RemoveConnectUserFromString(szVnodeName);
          RemoveGWNameFromString(szVnodeName);
          if (!IsIceServerLaunched(szVnodeName))
          {
            LoadString(hResource, IDS_TREE_ICE_SERVER_NONE,
                              bufRs, sizeof(bufRs));

            // Add sub-line < No Ice Server Running >
            lpRecord2 = AllocAndFillRecord(OT_ICE_SUBSTATICS, TRUE,
                                          lpRecord->extra,
                                          NULL,
                                          NULL,
                                          lpRecord->parentDbType,
                                          bufRs, bufOwner, bufComplim, NULL);
            if (!lpRecord2)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            idChildObj = TreeAddRecord(lpDomData, bufRs, idAnchorObj,
                                      0, 0, lpRecord2);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            // set back top item
            SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

            break;
          }
          }
          // static ice security
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_SECURITY, IDS_TREE_ICE_SECURITY_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice business units
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT, IDS_TREE_ICE_BUNIT_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice server
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_SERVER, IDS_TREE_ICE_SERVER_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ICE_SECURITY_SUBSTATICS:
          // static ice roles
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_ROLE, IDS_TREE_ICE_ROLE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice database users
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_DBUSER, IDS_TREE_ICE_DBUSER_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice database connections
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_DBCONNECTION, IDS_TREE_ICE_DBCONNECTION_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice web users
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_WEBUSER, IDS_TREE_ICE_WEBUSER_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice profiles
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_PROFILE, IDS_TREE_ICE_PROFILE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ICE_BUNIT_SUBSTATICS:
          // static business unit security
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_SECURITY, IDS_TREE_ICE_BUNIT_SECURITY_STATIC, FALSE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // Add 2 sub static items to idChildObj
          idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_SEC_ROLE, IDS_TREE_ICE_BUNIT_SEC_ROLE_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_SEC_USER, IDS_TREE_ICE_BUNIT_SEC_USER_STATIC, TRUE, TRUE);
          if (idChild2Obj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static business unit facets
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_FACET, IDS_TREE_ICE_BUNIT_FACET_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static business unit pages
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_PAGE, IDS_TREE_ICE_BUNIT_PAGE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static business unit locations
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        lpRecord->objName, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_BUNIT_LOCATION, IDS_TREE_ICE_BUNIT_LOCATION_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_ICE_SERVER_SUBSTATICS:
          // static ice server applications
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_SERVER_APPLICATION, IDS_TREE_ICE_SERVER_APPLICATION_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice server locations
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_SERVER_LOCATION, IDS_TREE_ICE_SERVER_LOCATION_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static ice server variables
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_ICE_SERVER_VARIABLE, IDS_TREE_ICE_SERVER_VARIABLE_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_INSTALL_SUBSTATICS:
          // static auditing
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_INSTALL_SECURITY, IDS_TREE_INSTALL_SECURITY_STATIC, FALSE, FALSE);    // no sub-dummy
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static grantees
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_INSTALL_GRANTEES, IDS_TREE_INSTALL_GRANTEES_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          // static alarms
          idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                        NULL, NULL, NULL,
                        DBTYPE_REGULAR,
                        NULL, NULL, NULL,
                        OT_STATIC_INSTALL_ALARMS, IDS_TREE_INSTALL_ALARMS_STATIC, TRUE, TRUE);
          if (idChildObj==0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal

          break;

        case OT_INSTALL_ALARMS_SUBSTATICS:
            // Sub branch CONNECT SUCCESS of DBALARM
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          NULL, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_CONN_SUCCESS,
                          IDS_TREE_DBALARM_CONN_SUCCESS_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch CONNECT FAILURE of DBALARM
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          NULL, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_CONN_FAILURE,
                          IDS_TREE_DBALARM_CONN_FAILURE_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch DISCONNECT SUCCESS of DBALARM
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          NULL, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_DISCONN_SUCCESS,
                          IDS_TREE_DBALARM_DISCONN_SUCCESS_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            
            // Sub branch DISCONNECT FAILURE of DBALARM
            idChildObj = AddStaticSubItem(lpDomData, idAnchorObj,
                          NULL, NULL, NULL,
                          lpRecord->parentDbType,
                          NULL, NULL, NULL,
                          OT_STATIC_DBALARM_DISCONN_FAILURE,
                          IDS_TREE_DBALARM_DISCONN_FAILURE_STATIC,
                          TRUE, TRUE);
            if (idChildObj==0)
              return MergeResStatus(iretGlob, RES_ERR);   // Fatal

            break;

        default:
          return RES_ERR;     // Forgotten case in the switch!
          break;
      }

      // mark original item as "with sub branch valid"
      lpRecord->bSubValid = TRUE;

      // set back top item
      SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

      return MergeResStatus(iretGlob, RES_SUCCESS);
      break;

    //
    // LOW-LEVEL OBJECTS
    //

    // level 0
    case OT_DATABASE:
    case OT_PROFILE:
    case OT_USER:
    case OT_GROUP:
    case OT_ROLE:
    case OT_LOCATION:

    // level 1, child of database
    case OTR_CDB:
    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
    case OT_SEQUENCE:
    case OT_SCHEMAUSER:
    case OT_SYNONYM:
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

    case OT_DBGRANT_SEQCRN_USER:
    case OT_DBGRANT_SEQCRY_USER:

    case OT_DBEVENT:
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:

    // level 1, child of database/replicator
    case OT_REPLIC_CONNECTION:
    case OT_REPLIC_CDDS:
    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_REGTABLE:

    // level 1, child of user
    case OTR_USERSCHEMA:
    case OTR_USERGROUP:
    case OTR_ALARM_SELSUCCESS_TABLE:
    case OTR_ALARM_SELFAILURE_TABLE:
    case OTR_ALARM_DELSUCCESS_TABLE:
    case OTR_ALARM_DELFAILURE_TABLE:
    case OTR_ALARM_INSSUCCESS_TABLE:
    case OTR_ALARM_INSFAILURE_TABLE:
    case OTR_ALARM_UPDSUCCESS_TABLE:
    case OTR_ALARM_UPDFAILURE_TABLE:

    case OTR_GRANTEE_RAISE_DBEVENT:
    case OTR_GRANTEE_REGTR_DBEVENT:
    case OTR_GRANTEE_EXEC_PROC:
    case OTR_GRANTEE_NEXT_SEQU:
    case OTR_DBGRANT_ACCESY_DB:
    case OTR_DBGRANT_ACCESN_DB:
    case OTR_DBGRANT_CREPRY_DB:
    case OTR_DBGRANT_CREPRN_DB:
    case OTR_DBGRANT_SEQCRY_DB:
    case OTR_DBGRANT_SEQCRN_DB:
    case OTR_DBGRANT_CRETBY_DB:
    case OTR_DBGRANT_CRETBN_DB:
    case OTR_DBGRANT_DBADMY_DB:
    case OTR_DBGRANT_DBADMN_DB:
    case OTR_DBGRANT_LKMODY_DB:
    case OTR_DBGRANT_LKMODN_DB:
    case OTR_DBGRANT_QRYIOY_DB:
    case OTR_DBGRANT_QRYION_DB:
    case OTR_DBGRANT_QRYRWY_DB:
    case OTR_DBGRANT_QRYRWN_DB:
    case OTR_DBGRANT_UPDSCY_DB:
    case OTR_DBGRANT_UPDSCN_DB:

    case OTR_DBGRANT_SELSCY_DB:
    case OTR_DBGRANT_SELSCN_DB:
    case OTR_DBGRANT_CNCTLY_DB:
    case OTR_DBGRANT_CNCTLN_DB:
    case OTR_DBGRANT_IDLTLY_DB:
    case OTR_DBGRANT_IDLTLN_DB:
    case OTR_DBGRANT_SESPRY_DB:
    case OTR_DBGRANT_SESPRN_DB:
    case OTR_DBGRANT_TBLSTY_DB:
    case OTR_DBGRANT_TBLSTN_DB:

    case OTR_DBGRANT_QRYCPY_DB:
    case OTR_DBGRANT_QRYCPN_DB:
    case OTR_DBGRANT_QRYPGY_DB:
    case OTR_DBGRANT_QRYPGN_DB:
    case OTR_DBGRANT_QRYCOY_DB:
    case OTR_DBGRANT_QRYCON_DB:

    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_CPI_TABLE:
    case OTR_GRANTEE_CPF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
    case OTR_GRANTEE_SEL_VIEW:
    case OTR_GRANTEE_INS_VIEW:
    case OTR_GRANTEE_UPD_VIEW:
    case OTR_GRANTEE_DEL_VIEW:
    case OTR_GRANTEE_ROLE:

    // level 1, child of group
    case OT_GROUPUSER:

    // level 1, child of role
    case OT_ROLEGRANT_USER:

    // level 1, child of location
    case OTR_LOCATIONTABLE:

    // level 2, child of "table of database"
    case OT_INTEGRITY:
    case OT_RULE:
    case OT_INDEX:
    case OT_TABLELOCATION:
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:

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

    case OTR_TABLESYNONYM:
    case OTR_TABLEVIEW:

    case OTR_REPLIC_TABLE_CDDS:

    // level 2, child of "view of database"
    case OT_VIEWTABLE:

    case OT_VIEWGRANT_SEL_USER:
    case OT_VIEWGRANT_INS_USER:
    case OT_VIEWGRANT_UPD_USER:
    case OT_VIEWGRANT_DEL_USER:

    case OTR_VIEWSYNONYM:

    // level 2, child of "procedure of database"
    case OT_PROCGRANT_EXEC_USER:
    case OTR_PROC_RULE:

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:

    // level 2, child of "dbevent of database"
    case OT_DBEGRANT_RAISE_USER:
    case OT_DBEGRANT_REGTR_USER:

    // level 2, child of "cdds of database"
    case OT_REPLIC_CDDS_DETAIL:
    case OTR_REPLIC_CDDS_TABLE:

    // level 2, child of "schema on database"
    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:

    // level 3, child of "index on table of database"
    case OTR_INDEXSYNONYM:

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_ALARMEE:
    //case OT_ALARMEE_SOLVED_USER           563
    //case OT_ALARMEE_SOLVED_GROUP          564
    //case OT_ALARMEE_SOLVED_ROLE           565
    case OT_S_ALARM_EVENT:

    // SPECIAL : VIRTNODE
    case OT_VIRTNODE:         // FULL NODE UPDATE - MIGHT BE RATHER LONG...

    // SPECIAL : OTLL_ (multiple updates)
    case OTLL_DBGRANTEE:
    case OTLL_GRANTEE:
    case OTLL_SECURITYALARM:
    case OTLL_DBSECALARM:
    case OTLL_OIDTDBGRANTEE:

    //
    // ICE
    //
    case OT_ICE_ROLE                 :
    case OT_ICE_DBUSER               :
    case OT_ICE_DBCONNECTION         :
    case OT_ICE_WEBUSER              :
    case OT_ICE_WEBUSER_ROLE         :
    case OT_ICE_WEBUSER_CONNECTION   :
    case OT_ICE_PROFILE              :
    case OT_ICE_PROFILE_ROLE         :
    case OT_ICE_PROFILE_CONNECTION   :
    // Under "Bussiness unit" (BUNIT)
    case OT_ICE_BUNIT                :
    case OT_ICE_BUNIT_SEC_ROLE       :
    case OT_ICE_BUNIT_SEC_USER       :
    case OT_ICE_BUNIT_FACET          :
    case OT_ICE_BUNIT_PAGE           :

    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_ROLE:
    case OT_ICE_BUNIT_PAGE_USER:

    case OT_ICE_BUNIT_LOCATION       :
    // Under "Server"
    case OT_ICE_SERVER_APPLICATION   :
    case OT_ICE_SERVER_LOCATION      :
    case OT_ICE_SERVER_VARIABLE      :

      // flag the "multiple updates" flag
      switch (iobjecttype) {
        case OT_VIRTNODE:
        case OTLL_DBGRANTEE:
        case OTLL_GRANTEE:
        case OTLL_SECURITYALARM:
        case OTLL_DBSECALARM:
        case OTLL_OIDTDBGRANTEE:
          bMultipleUpdates= TRUE;
          break;
        default:
          bMultipleUpdates= FALSE;
      }

      // 0) filter according to combination between iAction and iobjecttype
      if (!bMultipleUpdates) {
        switch (iAction) {
          case ACT_CHANGE_BASE_FILTER:
            if (iobjecttype != OT_DATABASE)
              return iretGlob;
            break;

          case ACT_CHANGE_OTHER_FILTER:
            if (!NonDBObjectHasOwner(iobjecttype))
              return iretGlob;
            break;

          case ACT_CHANGE_SYSTEMOBJECTS:
            if (!HasSystemObjects(GetBasicType(iobjecttype)))
              return iretGlob;
            break;
        }
      }

      // 1) Prepare variables according to object type :
      // level and string id for <no item>
      if (!bMultipleUpdates) {
        if (MakeLevelAndIdsNone(iobjecttype, &level, &ids_none) != RES_SUCCESS)
          return MergeResStatus(iretGlob, RES_ERR);

        // second switch on level, for aparents
        aparents[2] = NULL;   // default for third level
        switch (level) {
          case 0:
            aparents[0] = aparents[1] = NULL;   // parentstring not used
            break;
          case 1:
            if (parentstrings == NULL)
              aparents[0] = NULL;
            else
              aparents[0] = parentstrings[0]; // parent's name, may be NULL
            aparents[1] = NULL;
            break;
          case 2:
            if (parentstrings == NULL)
              aparents[0] = aparents[1] = NULL;
            else {
              aparents[0] = parentstrings[0]; // level 0 parent name, may be NULL
              aparents[1] = parentstrings[1]; // level 1 parent name, may be NULL
            }
            break;
          case 3:
            if (parentstrings == NULL)
              aparents[0] = aparents[1] = aparents[2] = NULL;
            else  {
              aparents[0] = parentstrings[0]; // base name, may be NULL
              aparents[1] = parentstrings[1]; // table name, may be NULL
              aparents[2] = parentstrings[2]; // index name, may be NULL
            }
            break;
          default:
            return MergeResStatus(iretGlob, RES_ERR);
        }
      }
      else {
        // Multiple updates
        level = 0;
        aparents[0] = aparents[1] = aparents[2] = NULL;
        irefobjecttype = iobjecttype;
      }

      // 2) Get first anchor id (current if expanditem)
      idAnchorObj = 0L;   // initial state

      // 2bis) Loop while idAnchorObj not null, on  steps 2ter to 7bis
      bRootItem = FALSE;  // for first call to GetAnchorId
      while (1) {
        if (bMultipleUpdates) {
          bRootItem = FALSE;
          idAnchorObj = GetAnyAnchorId(lpDomData, irefobjecttype,
                                       level, aparents,
                                       idAnchorObj, bufPar0, bufPar1, bufPar2,
                                       iAction, idItem,
                                       &iobjecttype, &level, &ids_none,
                                       bRootItem, &bRootItem);
        }
        else
          idAnchorObj = GetAnchorId(lpDomData, iobjecttype, level, aparents,
                                    idAnchorObj, bufPar0, bufPar1, bufPar2,
                                    iAction, idItem,
                                    bRootItem, &bRootItem);
        if (!idAnchorObj)
          break;    // break the "while 1"

        // special management for the "lonely" item
        if (bRootItem) {
          if (iAction != ACT_CHANGE_BASE_FILTER  &&
              iAction != ACT_CHANGE_OTHER_FILTER &&
              iAction != ACT_CHANGE_SYSTEMOBJECTS) {
            // Nothing to do, otherwise extra branches if an object
            // of the same type is added/altered/deleted

            // don't take the same item in account
            // at next call time to GetAnchorId
            bRootItem = FALSE;

            continue;   // loop on 2bis
          }

          // call DOMGetObject to check whether the object exists
          // or fits the filter
          lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);

          // anchor parenthood
          aparentsResult[0] = lpRecord->extra;    // level 0 parent
          aparentsResult[1] = lpRecord->extra2;   // level 1 parent
          aparentsResult[2] = lpRecord->extra3;   // level 2 parent

          // anchor aparentsResult2 on buffers
          aparentsResult2[0] = bufParResult02;
          aparentsResult2[1] = bufParResult12;
          aparentsResult2[2] = bufParResult22;

          // check the object existence - special code sequence
          // for a "system" feature
          iret = DOMGetObject(hnodestruct,
                              lpRecord->objName,
                              lpRecord->ownerName,  // object owner
                              iobjecttype,
                              level,                // level
                              aparentsResult,       // aparents
                              bwithsystem,
                              &resultType,
                              &resultLevel,
                              aparentsResult2,  // array of result parent strings
                              buf2,             // result object name
                              buf3,             // result owner
                              buf4);            // result complimentary data
          if (iret == RES_SUCCESS && !bwithsystem)
            if (IsSystemObject2(resultType, buf2, buf3, lpRecord->parentDbType))
              iret = RES_ENDOFDATA;   // system object, we don't want it!
            else
              iret = RES_SUCCESS;

          // check against owner filter
          if (iobjecttype == OT_DATABASE) {
              lpowner = lpDomData->lpBaseOwner;
          }
          else
            lpowner = lpDomData->lpOtherOwner;
          if (lpowner[0])
            if (x_strcmp(lpRecord->ownerName, lpowner))
              iret = RES_ENDOFDATA; // does not fit filter, we don't want it!

          if (iret != RES_SUCCESS) {
            TreeDeleteAllChildren(lpDomData, idAnchorObj);
          }
          else {
            // Emb 5/3/96 : make branch expandable only if has no children
            DWORD dwDummy;  // level, useless
            if (!GetFirstImmediateChild(lpDomData, idAnchorObj, &dwDummy)) {
              // say sub-branch not valid
              lpRecord->bSubValid = FALSE;

              // Add dummy sub-item
              if (AddDummySubItem(lpDomData, idAnchorObj)==0)
                return MergeResStatus(iretGlob, RES_ERR);   // Fatal

              // Set the sub item in full collapsed mode
              SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                          0, (LPARAM)idAnchorObj);

#ifdef  WIN95_CONTROLS
              // Force tree redraw for +/- and icon
              SetFocus(lpDomData->hwndTreeLb);
#endif  // WIN95_CONTROLS
            }
          }
          
          // don't take the same item in account
          // at next call time to GetAnchorId
          bRootItem = FALSE;

          continue;   // loop on 2bis
        }

        lpRecord4WildCard=(LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
 
        // 2bisbis) don't update not yet expanded branches for
        // certain values of iAction
        // TEST TO BE FINISHED - CHECK ALL IACTION VALUES
        if (iAction==ACT_CHANGE_BASE_FILTER         ||
            iAction==ACT_CHANGE_OTHER_FILTER        ||
            iAction==ACT_CHANGE_SYSTEMOBJECTS       ||
            iAction==ACT_BKTASK                     ||
            iAction==ACT_UPDATE_DEPENDANT_BRANCHES  ||
            iAction==ACT_ADD_OBJECT                 ||
            iAction==ACT_ALTER_OBJECT               ||
            iAction==ACT_DROP_OBJECT                ) {
          lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
          if (lpRecord->bSubValid == FALSE)
            continue;   // loop on 2bis
        }

        // 2ter) update aparentsTemp
        if (bufPar0[0] != '\0')
          aparentsTemp[0] = bufPar0;
        else
          aparentsTemp[0] = aparents[0];
        if (bufPar1[0] != '\0')
          aparentsTemp[1] = bufPar1;
        else
          aparentsTemp[1] = aparents[1];
        if (bufPar2[0] != '\0')
          aparentsTemp[2] = bufPar2;
        else
          aparentsTemp[2] = aparents[2];

        /* FINIR
        // Custom for new types following discussion with FNN
        switch (iobjecttype) {
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
          case OT_S_ALARM_CO_SUCCESS_USER :
          case OT_S_ALARM_CO_FAILURE_USER :
          case OT_S_ALARM_DI_SUCCESS_USER :
          case OT_S_ALARM_DI_FAILURE_USER :
            if (aparentsTemp[0] == NULL)
              aparentsTemp[0] = "";
        }
        */

        // 2quart) memorize top item
        idTop = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM,
                                   0, 0L);

#ifdef WIN32
        // masqued 30/05/97 since generic fix by emb in tree dll
        /*
        // 3 minus epsilon) insert a dummy sub-item before
        // the other sub-items, to prevent the bug in the tree
        // if we insert at first position
        idChildObj = GetFirstImmediateChild(lpDomData, idAnchorObj, &anchorLevel);
        lpRecord = AllocAndFillRecord(-1,  // Tested in FindImmediateSubItem to skip record
                                      FALSE,
                                      aparentsResult[0],
                                      NULL,
                                      NULL,
                                      "Error", "Error", "Error",
                                      NULL);
        if (!lpRecord)
          return MergeResStatus(iretGlob, RES_ERR);   // Fatal
        idAntiSmutObj = TreeAddRecord(lpDomData, "Error", idAnchorObj,
                                       0,          // idInsertAfter,
                                       idChildObj, // idInsertBefore,
                                       lpRecord);
        if (idAntiSmutObj==0)
          return MergeResStatus(iretGlob, RES_ERR);   // Fatal
        */
#endif // WIN32
        
        // 3) mark all old immediate children of the branch as to be deleted
        idChildObj = GetFirstImmediateChild(lpDomData, idAnchorObj, &anchorLevel);
        idLastOldChildObj = idChildObj;
        while (idChildObj) {
          lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idChildObj);
          lpRecord->bToBeDeleted = TRUE;
          idLastOldChildObj = idChildObj;
          idChildObj = GetNextImmediateChild(lpDomData, idAnchorObj,
                                            idChildObj, anchorLevel);
        }

        // 3bis) prepare parameters for DOMGetFirstObject call
        if (HasParent1WithSchema(iobjecttype)) {
          // query the anchor record data first
          lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
          if (lpRecord->tableOwner[0] != '\0') {
            x_strcpy(buf, aparentsTemp[1]);
            StringWithOwner(buf, lpRecord->tableOwner, aparentsTemp[1]);
          }
          if (HasParent2WithSchema(iobjecttype)) {
            x_strcpy(buf, aparentsTemp[2]);
            StringWithOwner(buf, lpRecord->ownerName, aparentsTemp[2]);
          }
        }
        if (iobjecttype == OT_DATABASE) {
            lpowner = lpDomData->lpBaseOwner;
        }
        else
          lpowner = lpDomData->lpOtherOwner;


        // FNN 080295 added loading of tables in the cache to avoid
        // system error if expading registered tables

        if (iobjecttype==OT_REPLIC_REGTABLE ||
            iobjecttype==OTR_REPLIC_CDDS_TABLE) {
           int iloc;
           DOMGetObjectLimitedInfo(hnodestruct,"ii_tables","$ingres",OT_TABLE,1,
              aparentsTemp, TRUE, &iloc,buf,buf,buf);
        }
        // FNN end of change

        time1 = time(NULL);

        // 4) loop on the brand new children
        if (IsRelatedType(iobjecttype)) {
          // re-anchor aparentsResult on buffers
          aparentsResult[0] = bufParResult0;
          aparentsResult[1] = bufParResult1;
          aparentsResult[2] = bufParResult2;
          CleanStringsArray(aparentsResult,
                          sizeof(aparentsResult)/sizeof(aparentsResult[0]));
          memset (&bufComplim, '\0', sizeof(bufComplim));
          memset (&bufOwner, '\0', sizeof(bufOwner));

          iret =  DOMGetFirstRelObject(hnodestruct,
                                       iobjecttype,
                                       level,
                                       aparentsTemp,   // Temp mandatory!
                                       bwithsystem,
                                       lpDomData->lpBaseOwner,
                                       lpDomData->lpOtherOwner,
                                       aparentsResult,
                                       buf,
                                       bufOwner,
                                       bufComplim);
        }
        else {
          memset (&bufComplim, '\0', sizeof(bufComplim));
          memset (&bufOwner, '\0', sizeof(bufOwner));

          iret =  DOMGetFirstObject(hnodestruct,
                                    iobjecttype,
                                    level,
                                    aparentsTemp,   // Temp mandatory!
                                    bwithsystem,
                                    lpowner,
                                    buf,
                                    bufOwner,
                                    bufComplim);
        }
        // Emb Sep. 7, 95: Cleanup of bufOwner moved in the while loop
        // so that it is made for DomGetNext() calls also
        bNoItem = TRUE ;

        time2 = time(NULL);

        // Initialize the search starting id for FindImmediateSubItem
        idFirstOldChildObj = 0L;    // Convention: none

        // get parent record in case it's data will be used
        lpAnchorRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                        LM_GETITEMDATA,
                                        0, (LPARAM)idAnchorObj);

        // start list of items under which a dummy sub-item has to be added
        if (!DumListInit())
          return MergeResStatus(iretGlob, RES_ERR); // Fatal

        while (iret != RES_ENDOFDATA) {
          // FNN 080295 added filter for wildcards
          if (!IsStringExpressionCompatible(buf,
                                           lpRecord4WildCard->wildcardFilter))
             goto loopcontinue;
          bNoItem = FALSE;
          // FNN 080295 end of change

          // Manage error cases
          if (iret==RES_ERR || iret==RES_TIMEOUT || iret==RES_NOGRANT) {
            // mark item as "with sub branch valid"
            // for any error type, so that force refresh will work!
            lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
            lpRecord->bSubValid = TRUE;   // valid branch

            // deleted all obsolete children
            TreeDeleteAllChildren(lpDomData, idAnchorObj);

            // create dummy sub item so the anchor is displayed correctly
            if (AddProblemSubItem(lpDomData, idAnchorObj, iret,
                                          iobjecttype, lpRecord) == 0)
              return MergeResStatus(iretGlob, RES_ERR); // Fatal

            iretGlob = MergeResStatus(iretGlob, iret);
            DumListFree();
            goto LoopGetAnchorId; // continue the for (idAnchorObj) loop
          }

          #ifdef DEBUGMALLOC
          {
            static int fracture = 50;
            itemCpt++;    // item count, for breakpoint only
            if (itemCpt % fracture == 0) {
              itemCpt++;
              itemCpt--;
            }
          }
          #endif  //DEBUGMALLOC

          // Here, we MUST have iret equal to RES_SUCCESS!

          // specific for star: a CDB is considered as system
          if (iobjecttype == OT_DATABASE && !bwithsystem) {
            databaseType = getint(bufComplim+STEPSMALLOBJ);
            if (databaseType == DBTYPE_COORDINATOR)
              goto loopcontinue;
          }


          // cleanup bufOwner/bufComplim
          // Added Sep. 11, 95 : Don't cleanup for related types
          if (!IsRelatedType(iobjecttype))
            if (iobjecttype!= OT_DATABASE &&
                !NonDBObjectHasOwner(iobjecttype) &&
                !ObjectUsesOwnerField(iobjecttype))
              bufOwner[0] = '\0';

          // mark the sub-item as not to be deleted if exists,
          // otherwise, add sub item
          if (IsRelatedType(iobjecttype))
            parent0 = aparentsResult[0];
          else
            parent0 = aparentsTemp[0];
          if ( bOldCode || iAction != ACT_EXPANDITEM)
            idChildObj = FindImmediateSubItem(lpDomData, idAnchorObj,
                                                        idFirstOldChildObj,
                                                        idLastOldChildObj,
                                                        iobjecttype,
                                                        buf,
                                                        bufOwner,
                                                        parent0,
                                                        bufComplim,
                                                        &idInsertAfter,
                                                        &idInsertBefore);
          else {
            // assume not found at first expansion
            idChildObj = 0;
            idInsertAfter = 0;
            idInsertBefore = 0;
          }

          // Manage passage from "system non checked" to "system checked":
          // should recreate the dummy sub-item if necessary
          if ( bOldCode || iAction != ACT_EXPANDITEM) {
            if (bwithsystem) {
              if (CanBeSystemObj(iobjecttype)) {
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                          LM_GETITEMDATA, 0, idChildObj);
                if (!lpRecord)
                  goto FixSchemaBug;  // fix Emb 07/11/96 : not found ---> continue in sequence
                  //return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                basicType = GetBasicType(iobjecttype);
                if (IsSystemObject2(basicType,
                                    lpRecord->objName,
                                    lpRecord->ownerName,
                                    lpRecord->parentDbType)) {
                  // create static sub items for special cases,
                  // or dummy sub-item for most cases
                  switch (iobjecttype) {
                    case OT_TABLELOCATION:
                      // Add static sub-item "Tables using the location"
                      // put the location name as extra data
                      idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                    lpRecord->objName,
                                    NULL,
                                    NULL,
                                    lpRecord->parentDbType,
                                    lpRecord->ownerName,
                                    lpRecord->szComplim,
                                    NULL,
                                    OT_STATIC_R_LOCATIONTABLE,
                                    IDS_TREE_R_LOCATIONTABLE_STATIC, TRUE, TRUE);
                      if (idChild2Obj==0)
                        return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                      break;
                    case OT_RULEPROC:
                      idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                    lpRecord->extra,      // parent1: database
                                    lpRecord->objName,    // parent2: procedure
                                    NULL,                 // parent3
                                    lpRecord->parentDbType,
                                    lpRecord->ownerName,  // owner
                                    lpRecord->szComplim,  // complim
                                    lpRecord->ownerName,  // ???
                                    OT_STATIC_PROCGRANT_EXEC_USER,
                                    IDS_TREE_PROCGRANT_EXEC_USER_STATIC, TRUE, TRUE);
                      if (idChild2Obj==0)
                        return MergeResStatus(iretGlob, RES_ERR);   // Fatal
      
                      idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                    lpRecord->extra,      // parent1: database
                                    lpRecord->objName,    // parent2: procedure
                                    NULL,                 // parent3
                                    lpRecord->parentDbType,
                                    lpRecord->ownerName,  // owner
                                    lpRecord->szComplim,  // complim
                                    lpRecord->ownerName,  // ???
                                    OT_STATIC_R_PROC_RULE,
                                    IDS_TREE_R_PROC_RULE_STATIC, TRUE, TRUE);
                       if (idChild2Obj==0)
                       return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                      break;
                    default:
                      // say sub-branch not valid
                      lpRecord->bSubValid = FALSE;
  
                      // Add dummy sub-item
                      if (AddDummySubItem(lpDomData, idChildObj)==0)
                        return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                  }

                  // Set the sub item in full collapsed mode
                  SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                              0, (LPARAM)idChildObj);
                }
              }
            }
          }

FixSchemaBug:

          // Added 24/3/95: if sub-item already exists and already
          // marked as "not to be deleted", let another item be created
          // So the end-user will see the duplicate line

          if (idChildObj) {
            // sub-item already exists : mark as not to be deleted
            lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idChildObj);
            if (lpRecord->bToBeDeleted == FALSE) {
              // Prepare to create another item next to the found one
              idInsertBefore = 0;
              idInsertAfter  = idChildObj;
              idChildObj     = 0;
            }
            else {
              BOOL bNeedsLineCaptionUpdate = FALSE;
              lpRecord->bToBeDeleted = FALSE;
              // and update sub-item complimentary and owner information
              // Emb Sep. 7, 95 : special managements when bufOwner
              // or bufComplim contains funny material
              switch (iobjecttype) {
                case OT_S_ALARM_SELSUCCESS_USER:  // alarms on tables
                case OT_S_ALARM_SELFAILURE_USER:
                case OT_S_ALARM_DELSUCCESS_USER:
                case OT_S_ALARM_DELFAILURE_USER:
                case OT_S_ALARM_INSSUCCESS_USER:
                case OT_S_ALARM_INSFAILURE_USER:
                case OT_S_ALARM_UPDSUCCESS_USER:
                case OT_S_ALARM_UPDFAILURE_USER:
                case OT_S_ALARM_CO_SUCCESS_USER:  // alarms on databases
                case OT_S_ALARM_CO_FAILURE_USER:
                case OT_S_ALARM_DI_SUCCESS_USER:
                case OT_S_ALARM_DI_FAILURE_USER:
                  // we received the following data:
                  // bufOwner ---> schema.dbevent associated with the alarm
                  // bufComplim[0] ---> alarm number
                  // bufComplim[STEPSMALLOBJ] ---> alarm name (optional)
                  x_strcpy(lpRecord->szComplim, bufComplim+STEPSMALLOBJ);
                  lpRecord->complimValue = (long)getint(bufComplim);
                  lstrcpy(lpRecord->ownerName, bufOwner);
                  break;

                case OT_INTEGRITY :
                  // integrity number in bufComplim
                  lpRecord->complimValue = (LONG)getint(bufComplim);
                  lpRecord->szComplim[0] = '\0';  // redundant
                  lstrcpy(lpRecord->ownerName, bufOwner);
                  break;

                case OTR_ALARM_SELSUCCESS_TABLE:
                case OTR_ALARM_SELFAILURE_TABLE:
                case OTR_ALARM_DELSUCCESS_TABLE:
                case OTR_ALARM_DELFAILURE_TABLE:
                case OTR_ALARM_INSSUCCESS_TABLE:
                case OTR_ALARM_INSFAILURE_TABLE:
                case OTR_ALARM_UPDSUCCESS_TABLE:
                case OTR_ALARM_UPDFAILURE_TABLE:
                  // related security number in bufComplim
                  lpRecord->complimValue = (long)getint(bufComplim);
                  lpRecord->szComplim[0] = '\0';
                  lstrcpy(lpRecord->ownerName, bufOwner);
                  break;

                case OT_REPLIC_CONNECTION:
                  // connection security number in bufOwner
                  lpRecord->complimValue = (long)getint(bufOwner);
                  lpRecord->ownerName[0] = '\0';  // redundant at this time
                  lstrcpy(lpRecord->szComplim, bufComplim); // regular use
                  if (x_stricmp(lpRecord->objName, buf) != 0) {
                    lstrcpy(lpRecord->objName, buf);
                    bNeedsLineCaptionUpdate = TRUE;
                  }
                  break;

                case OT_REPLIC_CDDS:
                case OTR_REPLIC_TABLE_CDDS:
                  // CDDS number in bufOwner
                  lpRecord->complimValue = (long)getint(bufOwner);
                  lpRecord->ownerName[0] = '\0';  // redundant at this time
                  lstrcpy(lpRecord->szComplim, bufComplim); // regular use
                  break;

                default:
                  // regular use of bufComplim and bufOwner
                  lstrcpy(lpRecord->szComplim, bufComplim);
                  lstrcpy(lpRecord->ownerName, bufOwner);
              }

              if (bNeedsLineCaptionUpdate) {
                SETSTRING setString;    // for tree line caption update
                setString.lpString  = lpRecord->objName;
                setString.ulRecID   = idChildObj;
                SendMessage(lpDomData->hwndTreeLb, LM_SETSTRING, 0,
                            (LPARAM) (LPSETSTRING)&setString );
              }

              // "Advance" for the next FindImmediateSubItem (sort assumed)
              idFirstOldChildObj = idChildObj;
            }
          }

          // Create item if not-existing or duplicate
          if (!idChildObj) {
            switch (iobjecttype) {
              case OTR_CDB:
              case OT_DATABASE:
              case OTR_USERSCHEMA:
              case OTR_DBGRANT_ACCESY_DB:
              case OTR_DBGRANT_ACCESN_DB:
              case OTR_DBGRANT_CREPRY_DB:
              case OTR_DBGRANT_CREPRN_DB:
              case OTR_DBGRANT_SEQCRY_DB:
              case OTR_DBGRANT_SEQCRN_DB:
              case OTR_DBGRANT_CRETBY_DB:
              case OTR_DBGRANT_CRETBN_DB:
              case OTR_DBGRANT_DBADMY_DB:
              case OTR_DBGRANT_DBADMN_DB:
              case OTR_DBGRANT_LKMODY_DB:
              case OTR_DBGRANT_LKMODN_DB:
              case OTR_DBGRANT_QRYIOY_DB:
              case OTR_DBGRANT_QRYION_DB:
              case OTR_DBGRANT_QRYRWY_DB:
              case OTR_DBGRANT_QRYRWN_DB:
              case OTR_DBGRANT_UPDSCY_DB:
              case OTR_DBGRANT_UPDSCN_DB:

              case OTR_DBGRANT_SELSCY_DB:
              case OTR_DBGRANT_SELSCN_DB:
              case OTR_DBGRANT_CNCTLY_DB:
              case OTR_DBGRANT_CNCTLN_DB:
              case OTR_DBGRANT_IDLTLY_DB:
              case OTR_DBGRANT_IDLTLN_DB:
              case OTR_DBGRANT_SESPRY_DB:
              case OTR_DBGRANT_SESPRN_DB:
              case OTR_DBGRANT_TBLSTY_DB:
              case OTR_DBGRANT_TBLSTN_DB:

              case OTR_DBGRANT_QRYCPY_DB:
              case OTR_DBGRANT_QRYCPN_DB:
              case OTR_DBGRANT_QRYPGY_DB:
              case OTR_DBGRANT_QRYPGN_DB:
              case OTR_DBGRANT_QRYCOY_DB:
              case OTR_DBGRANT_QRYCON_DB:

                // Sub item: we say it's subdata is not valid
                // since the static sub-sub-items (tables/views/...)
                // will be created on the fly when the item will be expanded

                // Pick database type HERE!
                databaseType = getint(bufComplim+STEPSMALLOBJ);

                if (!IsRelatedType(iobjecttype))
                  lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                                aparentsTemp[0],
                                                aparentsTemp[1],
                                                aparentsTemp[2],
                                                databaseType,
                                                buf,
                                                bufOwner,
                                                bufComplim, NULL);
                else
                  lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                databaseType,
                                                buf,
                                                bufOwner,
                                                bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OT_PROFILE:
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              NULL, NULL, NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore,
                                           lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // No dummy sub-item for this kind of sub item
                break;

              case OT_USER:
              case OT_GROUPUSER:    // Merged with OT_USER 10/02/95
                // Sub item: we say it's subdata is not valid :
                // the static sub-sub-items will be created at expand time
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsTemp[0],
                                              aparentsTemp[1],
                                              aparentsTemp[2],
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Restricted features if gateway
                if (GetOIVers() != OIVERS_NOTOI) {
                  // add in list of candidates for dummy items
                  if (!DumListAdd(idChildObj)) {
                    DumListFree();
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                  }
                }

                break;

              case OT_ROLEGRANT_USER:
                // get role name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // role name
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OT_S_ALARM_SELSUCCESS_USER:  // alarms on tables
              case OT_S_ALARM_SELFAILURE_USER:
              case OT_S_ALARM_DELSUCCESS_USER:
              case OT_S_ALARM_DELFAILURE_USER:
              case OT_S_ALARM_INSSUCCESS_USER:
              case OT_S_ALARM_INSFAILURE_USER:
              case OT_S_ALARM_UPDSUCCESS_USER:
              case OT_S_ALARM_UPDFAILURE_USER:
              case OT_S_ALARM_CO_SUCCESS_USER:  // alarms on databases
              case OT_S_ALARM_CO_FAILURE_USER:
              case OT_S_ALARM_DI_SUCCESS_USER:
              case OT_S_ALARM_DI_FAILURE_USER:
                // we received the following data:
                // buf ---> alarmee name (can be user/group/role)
                // bufOwner ---> schema.dbevent associated with the alarm
                // bufComplim[0] ---> alarm number
                // bufComplim[STEPSMALLOBJ] ---> alarm name (optional)

                // Sub item: we say it's subdata is not valid :
                // the static sub-sub-items will be created at expand time
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsTemp[0],
                                              aparentsTemp[1],
                                              aparentsTemp[2],
                                              lpAnchorRecord->parentDbType,
                                              buf,
                                              bufOwner,
                                              bufComplim + STEPSMALLOBJ,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // 13/3/95 : store security number with getint
                lpRecord->complimValue = (long)getint(bufComplim);
                bufComplim[0] = '\0';

                // Make line caption : (alarmid/alarmname) alarmeename
                // or (alarmid) alarmeename if no alarmname
                MakeSecurityCaption(buf, lpRecord->complimValue,
                                         lpRecord->szComplim);

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OT_VIEWTABLE:
                // get base name and table/view name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpRecord->extra2); // table/view
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // if gateway, no sub-branch anymore
                if (GetOIVers()==OIVERS_NOTOI)
                    break;  // don't add the dummy sub-item

				// add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OT_INTEGRITY :
                // get base name and table name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpRecord->extra2); // table name
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              lpRecord->tableOwner);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // 13/3/95 : store integrity number with getint
                lpRecord->complimValue = (LONG)getint(bufComplim);

                lpRecord->szComplim[0] = bufComplim[0] = '\0';

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                break;

              case OT_RULE:
              case OTR_PROC_RULE:
                  bSubDummy = TRUE;

                if (!IsRelatedType(iobjecttype)) {
                  // get base name and table name
                  // (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  lstrcpy(aparentsResult[1], lpRecord->extra2); // table name
                  lpRecord = AllocAndFillRecord(iobjecttype, bSubDummy,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                NULL,
                                                lpRecord->parentDbType,
                                                buf, bufOwner, bufComplim,
                                                lpRecord->tableOwner);
                }
                else
                  lpRecord = AllocAndFillRecord(OT_RULE, bSubDummy,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                lpAnchorRecord->parentDbType,
                                                buf,
                                                bufOwner,
                                                bufComplim,
                                                NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Add static sub-item RULEPROC
                // put base name as first extra data,
                // table name as second extra data,
                // and rule name as third extra data.
                {
                  idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              buf,
                                              lpRecord->parentDbType,
                                              bufOwner, bufComplim,
                                              NULL, // SCHEMA ???
                                              OT_STATIC_RULEPROC,
                                              IDS_TREE_RULEPROC_STATIC, TRUE, TRUE);
                   if (idChild2Obj==0)
                   return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                   // Set the sub item in full collapsed mode
                   SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                               0, (LPARAM)idChildObj);
                }
                break;

              case OT_TABLELOCATION:
                // get base name and table/view name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpRecord->extra2); // table/view
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              lpRecord->tableOwner);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Add static sub-item "Tables using the location"
                // put the location name as extra data
                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf,
                              NULL,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim, NULL,
                              OT_STATIC_R_LOCATIONTABLE,
                              IDS_TREE_R_LOCATIONTABLE_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);
                break;

              case OT_INDEX:
                // get base name and table name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpRecord->extra2); // table/view
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              lpRecord->tableOwner);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // if star index, no sub branch
                if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
                  break;

                // static INDEXSYNONYMS item creation delayed at expand time
                // Add a dummy sub-item in replacement for expand feature

                if (lpDomData->ingresVer!=OIVERS_NOTOI) {
                  // add in list of candidates for dummy items
                  if (!DumListAdd(idChildObj)) {
                    DumListFree();
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                  }
                }

                break;

              case OT_SCHEMAUSER:
                // get base name (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add a dummy sub-item since it is like a user
                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                // No dummy sub-sub-item for this kind of sub items
                break;

              case OT_SYNONYM:
              case OTR_TABLESYNONYM:
              case OTR_VIEWSYNONYM:
              case OTR_INDEXSYNONYM:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  lstrcpy(aparentsResult[1], lpRecord->extra2); // table/view
                  lstrcpy(aparentsResult[2], lpRecord->extra3); // (index)
                }
                else {
                  // aparentsResult[0] to [2] already known -
                  // we use the 3 levels :
                  // aparentsResult[0] = base name
                  // aparentsResult[1] = table/view name
                  // aparentsResult[2] = index name (if synonym on index)
                }
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              aparentsResult[2],
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);    // SCHEMA ???
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              aparentsResult[0],
                              bufComplim,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim,
                              NULL,   // SCHEMA ???
                              OT_STATIC_SYNONYMED,
                              IDS_TREE_SYNONYMED_STATIC, FALSE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Add a sub-item to the static : the synonymed object
                // (type not known yet)
                // 29/6/95 : store the owner name for the object
                lpRecord = AllocAndFillRecord(OT_SYNONYMOBJECT, FALSE,
                                          aparentsResult[0], NULL, NULL,
                                          lpRecord->parentDbType,
                                          bufComplim,
                                          bufOwner,
                                          NULL,
                                          OwnerFromString(bufComplim, buf));
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                // 29/6/95 : caption: remove the owner name for the object
                idChild3Obj = TreeAddRecord(lpDomData,
                                            StringWithoutOwner(bufComplim),
                                            idChild2Obj,
                                            idInsertAfter, idInsertBefore,
                                            lpRecord);
                if (idChild3Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add a dummy sub-sub-item to the synonymed object tree line
                // add in list of candidates for dummy items
                if (!DumListAdd(idChild3Obj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                // Set the sub-sub item in fast collapsed mode
                // (fast mode can be used since we have a DumList)
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0xdcba, (LPARAM)idChild2Obj);

                // Set the sub item in fast collapsed mode
                // (fast mode can be used since we have a DumList)
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0xdcba, (LPARAM)idChildObj);
                break;

              case OT_PROCEDURE:
              case OT_RULEPROC:
              case OTR_GRANTEE_EXEC_PROC:
              case OT_SCHEMAUSER_PROCEDURE:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                NULL,
                                                NULL,
                                                lpRecord->parentDbType,
                                                buf, bufOwner, bufComplim,
                                                bufOwner);  // Procedure owner
                }
                else
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                lpAnchorRecord->parentDbType,
                                                buf,
                                                bufOwner,
                                                bufComplim,
                                                bufOwner);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // build an object caption with the base name if related
                x_strcpy(buf2, buf);
                if (IsRelatedType(iobjecttype))
                  MakeRelatedLevel1ObjCaption(buf2, aparentsResult[0]);

                idChildObj = TreeAddRecord(lpDomData, buf2, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal


                // if star procedure, no sub branch
                if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
                  break;

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              aparentsResult[0],
                              buf,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim, bufOwner,
                              OT_STATIC_PROCGRANT_EXEC_USER,
                              IDS_TREE_PROCGRANT_EXEC_USER_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                {
                  idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                aparentsResult[0],
                                buf,
                                NULL,
                                lpRecord->parentDbType,
                                bufOwner, bufComplim, bufOwner,
                                OT_STATIC_R_PROC_RULE,
                                IDS_TREE_R_PROC_RULE_STATIC, TRUE, TRUE);
                   if (idChild2Obj==0)
                   return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                   // Set the sub item in full collapsed mode
                   SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                               0, (LPARAM)idChildObj);
                }

                // Parent branch if xref
                if (IsRelatedType(iobjecttype))
                  AddParentBranch(lpDomData, idChildObj);

                break;

              case OT_SEQUENCE:
              case OTR_GRANTEE_NEXT_SEQU:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                NULL,
                                                NULL,
                                                lpRecord->parentDbType,
                                                buf, bufOwner, bufComplim,
                                                bufOwner);  // Procedure owner
                }
                else
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                lpAnchorRecord->parentDbType,
                                                buf,
                                                bufOwner,
                                                bufComplim,
                                                bufOwner);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // build an object caption with the base name if related
                x_strcpy(buf2, buf);
                if (IsRelatedType(iobjecttype))
                  MakeRelatedLevel1ObjCaption(buf2, aparentsResult[0]);

                idChildObj = TreeAddRecord(lpDomData, buf2, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal



                // if star procedure, no sub branch
                if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
                  break;

                if ( GetOIVers() >= OIVERS_30 )
                {
                  idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                aparentsResult[0],
                                buf,
                                NULL,
                                lpRecord->parentDbType,
                                bufOwner, bufComplim, bufOwner,
                                OT_STATIC_SEQGRANT_NEXT_USER,
                                IDS_TREE_SEQUENCE_GRANT_STATIC, TRUE, TRUE);
                  if (idChild2Obj==0)
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                  // Parent branch if xref
                  if (IsRelatedType(iobjecttype))
                    AddParentBranch(lpDomData, idChildObj);
                }

                break;

              case OT_DBEVENT:      // desktop : used for Events
              case OT_S_ALARM_EVENT:
              case OTR_GRANTEE_RAISE_DBEVENT:
              case OTR_GRANTEE_REGTR_DBEVENT:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                NULL,
                                                NULL,
                                                lpRecord->parentDbType,
                                                buf, bufOwner, bufComplim,
                                                NULL);
                }
                else
                  lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                lpAnchorRecord->parentDbType,
                                                buf,
                                                bufOwner,
                                                bufComplim,
                                                NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // build an object caption with the base name if related
                x_strcpy(buf2, buf);
                if (IsRelatedType(iobjecttype))
                  MakeRelatedLevel1ObjCaption(buf2, aparentsResult[0]);

                idChildObj = TreeAddRecord(lpDomData, buf2, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // DBevent enabled for this Gateway but no dbevent grantee
                // available (ORACLE)
                if (iobjecttype == OT_DBEVENT && GetOIVers() == OIVERS_NOTOI)
                    break; 

                // sub branches for open ingres without gateway
                  idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                aparentsResult[0],
                                buf,
                                NULL,
                                lpRecord->parentDbType,
                                bufOwner, bufComplim, bufOwner,
                                OT_STATIC_DBEGRANT_RAISE_USER,
                                IDS_TREE_DBEGRANT_RAISE_USER_STATIC, TRUE, TRUE);
                  if (idChild2Obj==0)
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                  idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                                aparentsResult[0],
                                buf,
                                NULL,
                                lpRecord->parentDbType,
                                bufOwner, bufComplim, bufOwner,
                                OT_STATIC_DBEGRANT_REGTR_USER,
                                IDS_TREE_DBEGRANT_REGTR_USER_STATIC, TRUE, TRUE);
                  if (idChild2Obj==0)
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                  // Set the sub item in full collapsed mode
                  SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                              0, (LPARAM)idChildObj);

                  // Parent branch if xref
                  if (IsRelatedType(iobjecttype))
                    AddParentBranch(lpDomData, idChildObj);

                break;

              case OT_ROLE:
              case OTR_GRANTEE_ROLE:
                // nothing in extra data, and no parenthood
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              NULL, NULL, NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // static R_GRANTS item creation delayed at expand time
                // Add a dummy sub-item in replacement for expand feature
                if (AddDummySubItem(lpDomData, idChildObj)==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);

                break;

              case OT_LOCATION:
                // nothing in extra data
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              NULL, NULL, NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Add static sub-item "Tables using the location"
                // put the location name as extra data
                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf,
                              NULL,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim, NULL,
                              OT_STATIC_R_LOCATIONTABLE,
                              IDS_TREE_R_LOCATIONTABLE_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);
                break;


              case OT_GROUP:
              case OTR_USERGROUP:
                // parent's name in extra data (group name)
                if (!IsRelatedType(iobjecttype))
                  lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                                buf, NULL, NULL,
                                                lpAnchorRecord->parentDbType,
                                                buf, NULL, bufComplim, NULL);
                else
                  lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                                aparentsResult[0],
                                                aparentsResult[1],
                                                aparentsResult[2],
                                                lpAnchorRecord->parentDbType,
                                                buf,
                                                bufOwner,
                                                bufComplim,
                                                NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // static GROUPUSERS and R_GRANTS items creation
                // delayed at expand time
                // Add a dummy sub-item in replacement for expand feature
                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OT_VIEW:
              case OTR_TABLEVIEW:
              case OTR_GRANTEE_SEL_VIEW:
              case OTR_GRANTEE_INS_VIEW:
              case OTR_GRANTEE_UPD_VIEW:
              case OTR_GRANTEE_DEL_VIEW:
              case OT_SCHEMAUSER_VIEW:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  // and put in aparentsResult
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                  aparentsResult[1] = NULL;
                  aparentsResult[2] = NULL;
                }
                else {
                  ;     // use aparentsResult "as is"
                }

                //
                // Sub item: we say it's subdata is not valid
                // since the static sub-sub-items will be created
                // on the fly when the item will be expanded
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              aparentsResult[2],
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // build an object caption with the base name if related
                // except if view referencing table
                x_strcpy(buf2, buf);
                if (IsRelatedType(iobjecttype)
                    && iobjecttype!=OTR_TABLEVIEW )
                  MakeRelatedLevel1ObjCaption(buf2, aparentsResult[0]);

                idChildObj = TreeAddRecord(lpDomData, buf2, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Except if star link view, which has no subitem:
                // Add a dummy sub-sub-item to this sub item
                // add in list of candidates for dummy items
                objType = getint(lpRecord->szComplim + 4 + STEPSMALLOBJ);
                if (objType == OBJTYPE_STARLINK)
                  break;
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OTR_ALARM_SELSUCCESS_TABLE:
              case OTR_ALARM_SELFAILURE_TABLE:
              case OTR_ALARM_DELSUCCESS_TABLE:
              case OTR_ALARM_DELFAILURE_TABLE:
              case OTR_ALARM_INSSUCCESS_TABLE:
              case OTR_ALARM_INSFAILURE_TABLE:
              case OTR_ALARM_UPDSUCCESS_TABLE:
              case OTR_ALARM_UPDFAILURE_TABLE:
                // base name already in aparentsResult[0] since all related

                // table family : one parent = basename
                // say subdata invalid because we will create the
                // static sub items at expand time
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // 14/3/95 : store related security number with getint
                lpRecord->complimValue = (long)getint(bufComplim);
                lpRecord->szComplim[0] = '\0';
                bufComplim[0] = '\0';

                // Make line caption : (integrity id) [basename]tablename
                MakeCrossSecurityCaption(buf, aparentsResult[0],
                                              lpRecord->complimValue);

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add in list of candidates for dummy items
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }

                break;

              case OTR_LOCATIONTABLE:
              case OT_TABLE:
              case OTR_GRANTEE_SEL_TABLE:
              case OTR_GRANTEE_INS_TABLE:
              case OTR_GRANTEE_UPD_TABLE:
              case OTR_GRANTEE_DEL_TABLE:
              case OTR_GRANTEE_REF_TABLE:
              case OTR_GRANTEE_CPI_TABLE:
              case OTR_GRANTEE_CPF_TABLE:
              case OTR_GRANTEE_ALL_TABLE:
              case OT_REPLIC_REGTABLE:
              case OTR_REPLIC_CDDS_TABLE:
              case OT_SCHEMAUSER_TABLE:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base
                }
                // else : base name already in aparentsResult[0]

                // table : one parent = basename
                // say subdata invalid because we will create the
                // static sub items at expand time
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // build an object caption with the base name if related
                // except if cdds_table
                x_strcpy(buf2, buf);
                if (IsRelatedType(iobjecttype))
                  if (iobjecttype != OTR_REPLIC_CDDS_TABLE)
                    MakeRelatedLevel1ObjCaption(buf2, aparentsResult[0]);

                idChildObj = TreeAddRecord(lpDomData, buf2, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // if gateway, no sub-branch anymore
                if (GetOIVers()==OIVERS_NOTOI)
                    break;  // don't add the dummy sub-item

                // only for replicator registered tables:
                // check for existence, and don't add dummy sub item
                // if does not exist.
                if (iobjecttype == OT_REPLIC_REGTABLE || iobjecttype == OTR_REPLIC_CDDS_TABLE) {
                  aparentsResult[0] = lpRecord->extra;    // level 0 parent
                  aparentsResult[1] = NULL;
                  aparentsResult[2] = NULL;

                  // anchor aparentsResult2 on buffers
                  aparentsResult2[0] = bufParResult02;
                  aparentsResult2[1] = bufParResult12;
                  aparentsResult2[2] = bufParResult22;

                  iret = DOMGetObject(hnodestruct,
                                      lpRecord->objName,
                                      lpRecord->ownerName,  // object owner
                                      OT_TABLE,
                                      1,                    // level
                                      aparentsResult,       // aparents
                                      FALSE,
                                      &resultType,
                                      &resultLevel,
                                      aparentsResult2,  // array of result parent strings
                                      buf3,             // result object name
                                      bufOwner,         // result owner
                                      bufComplim);      // result complimentary data
                  if (iret == RES_ENDOFDATA) {
                    // change caption
                    SETSTRING setString;

                    LoadString(hResource,
                              IDS_TREE_REPLICTABLE_NONEXISTENT,
                              buf3, sizeof(buf3));
                    wsprintf(buf4, buf3, buf2);

                    setString.lpString  = buf4;
                    setString.ulRecID   = idChildObj;
                    SendMessage(lpDomData->hwndTreeLb, LM_SETSTRING, 0,
                                (LPARAM) (LPSETSTRING)&setString );

                    break;  // don't add the dummy sub-item
                  }
                }

                // Except if star native table, which has no subitem:
                // Add a dummy sub-sub-item to this sub item
                // add in list of candidates for dummy items
                objType = getint(lpRecord->szComplim + STEPSMALLOBJ);
                if (objType == OBJTYPE_STARNATIVE)
                  break;
                if (!DumListAdd(idChildObj)) {
                  DumListFree();
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                }
                break;

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

              case OT_DBGRANT_SEQCRN_USER:
              case OT_DBGRANT_SEQCRY_USER:

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

              case OT_PROCGRANT_EXEC_USER:

              case OT_SEQUGRANT_NEXT_USER:

              case OT_DBEGRANT_RAISE_USER:
              case OT_DBEGRANT_REGTR_USER:
                // get base name and table/view/proc/dbevent name
                // (contained in extra of the anchor obj)
                // and put in aparentsResult
                // optimized : parent record read once before the loop
                lstrcpy(aparentsResult[0], lpAnchorRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpAnchorRecord->extra2); // tbl/view/none
                // 24/02/95 : replaced OT_GRANTEE by full type
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              // Emb 07/11/96 : lpRecord->tableOwner made a gpf
                                              lpAnchorRecord->tableOwner);  // SCHEMA IN CASE OF TABLE
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add in list of candidates for dummy items
                // if no gateway
                  if (!DumListAdd(idChildObj)) {
                    DumListFree();
                    return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                  }
                  // old code:
                  //if (AddDummySubItem(lpDomData, idChildObj)==0)
                  //  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                  //SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  //            0, (LPARAM)idChildObj);

                break;

              case OT_REPLIC_CONNECTION:
                // get base name (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name

                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // store connection security number with getint
                lpRecord->complimValue = (long)getint(bufOwner);
                lpRecord->ownerName[0] = '\0';
                bufOwner[0] = '\0';

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Terminal branch -
                // No dummy sub-sub-item for this kind of sub items
                break;

              case OT_REPLIC_MAILUSER:
                // get base name (contained in extra of the anchor obj)
                // and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Terminal branch -
                // No dummy sub-sub-item for this kind of sub items
                break;


              case OT_REPLIC_CDDS:
              case OTR_REPLIC_TABLE_CDDS:
                if (!IsRelatedType(iobjecttype)) {
                  // get base name (contained in extra of the anchor obj)
                  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                  LM_GETITEMDATA,
                                                  0, (LPARAM)idAnchorObj);
                  lstrcpy(aparentsResult[0], lpRecord->extra);  // base
                }
                // else : base name already in aparentsResult[0]

                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsResult[0],
                                              NULL,
                                              NULL,
                                              lpAnchorRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // store CDDS number with getint
                lpRecord->complimValue = (long)getint(bufOwner);
                lpRecord->ownerName[0] = '\0';
                bufOwner[0] = '\0';

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              aparentsResult[0],
                              buf,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim, NULL,
                              OT_STATIC_REPLIC_CDDS_DETAIL,
                              IDS_TREE_REPLIC_CDDS_DETAIL_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              aparentsResult[0],
                              buf,
                              NULL,
                              lpRecord->parentDbType,
                              bufOwner, bufComplim, NULL,
                              OT_STATIC_R_REPLIC_CDDS_TABLE,
                              IDS_TREE_R_REPLIC_CDDS_TABLE_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);

                break;

              case OT_REPLIC_CDDS_DETAIL:
                // get base name (contained in extra of the anchor obj)
                // and cdds name, and put in aparentsResult
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                LM_GETITEMDATA,
                                                0, (LPARAM)idAnchorObj);
                lstrcpy(aparentsResult[0], lpRecord->extra);  // base name
                lstrcpy(aparentsResult[1], lpRecord->extra2); // cdds name
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsResult[0],
                                              aparentsResult[1],
                                              NULL,
                                              lpRecord->parentDbType,
                                              buf, bufOwner, bufComplim,
                                              NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Note : complim data contains 3 values that will be
                // used for sort/search

                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                          idInsertAfter, idInsertBefore,
                                          lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Terminal branch -
                // No dummy sub-sub-item for this kind of sub items
                break;

              //
              // ICE
              //
              case OT_ICE_ROLE                 :
              case OT_ICE_DBUSER               :
              case OT_ICE_DBCONNECTION         :
              case OT_ICE_SERVER_APPLICATION   :
              case OT_ICE_SERVER_LOCATION      :
              case OT_ICE_SERVER_VARIABLE      :
                // No parent, no sub branch
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                break;

              case OT_ICE_WEBUSER              :
                // No parent, 2 subbranches
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf, NULL, NULL,
                              DBTYPE_REGULAR,
                              NULL, NULL, NULL,
                              OT_STATIC_ICE_WEBUSER_ROLE, IDS_TREE_ICE_WEBUSER_ROLE_STATIC,
                              TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf, NULL, NULL,
                              DBTYPE_REGULAR,
                              NULL, NULL, NULL,
                              OT_STATIC_ICE_WEBUSER_CONNECTION, IDS_TREE_ICE_WEBUSER_CONNECTION_STATIC,
                              TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                break;

              case OT_ICE_PROFILE              :
                // No parent, 2 subbranches
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf, NULL, NULL,
                              DBTYPE_REGULAR,
                              NULL, NULL, NULL,
                              OT_STATIC_ICE_PROFILE_ROLE, IDS_TREE_ICE_PROFILE_ROLE_STATIC,
                              TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              buf, NULL, NULL,
                              DBTYPE_REGULAR,
                              NULL, NULL, NULL,
                              OT_STATIC_ICE_PROFILE_CONNECTION, IDS_TREE_ICE_PROFILE_CONNECTION_STATIC,
                              TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                break;

              case OT_ICE_WEBUSER_ROLE         :
              case OT_ICE_WEBUSER_CONNECTION   :
              case OT_ICE_PROFILE_ROLE         :
              case OT_ICE_PROFILE_CONNECTION   :
              case OT_ICE_BUNIT_LOCATION       :
                // One parent, no sub branch
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsTemp[0],
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                break;

              case OT_ICE_BUNIT_FACET          :
                // One parent, 2 static sub branches
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsTemp[0],
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              lpRecord->extra,
                              buf,
                              NULL,
                              DBTYPE_REGULAR,
                              bufOwner, bufComplim, bufOwner,
                              OT_STATIC_ICE_BUNIT_FACET_ROLE,
                              IDS_TREE_ICE_BUNIT_FACET_ROLE_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              lpRecord->extra,
                              buf,
                              NULL,
                              DBTYPE_REGULAR,
                              bufOwner, bufComplim, bufOwner,
                              OT_STATIC_ICE_BUNIT_FACET_USER,
                              IDS_TREE_ICE_BUNIT_FACET_USER_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);
                break;

              case OT_ICE_BUNIT_PAGE           :
                // One parent, 2 static sub branches
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsTemp[0],
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              lpRecord->extra,
                              buf,
                              NULL,
                              DBTYPE_REGULAR,
                              bufOwner, bufComplim, bufOwner,
                              OT_STATIC_ICE_BUNIT_PAGE_ROLE,
                              IDS_TREE_ICE_BUNIT_PAGE_ROLE_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
                              lpRecord->extra,
                              buf,
                              NULL,
                              DBTYPE_REGULAR,
                              bufOwner, bufComplim, bufOwner,
                              OT_STATIC_ICE_BUNIT_PAGE_USER,
                              IDS_TREE_ICE_BUNIT_PAGE_USER_STATIC, TRUE, TRUE);
                if (idChild2Obj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);
                break;


              case OT_ICE_BUNIT_FACET_ROLE:
              case OT_ICE_BUNIT_FACET_USER:
              case OT_ICE_BUNIT_PAGE_ROLE:
              case OT_ICE_BUNIT_PAGE_USER:
                // Two parents, no sub branch
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsTemp[0],
                                              aparentsTemp[1],
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                break;


              case OT_ICE_BUNIT                :
                // No parent, dummy sub branch with OT_ICE_BUNIT_SUBSTATICS
                lpRecord = AllocAndFillRecord(iobjecttype, FALSE,
                                              NULL,
                                              NULL,
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // add a dummy sub-item
                if (AddDummySubItem(lpDomData, idChildObj)==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal

                // Set the sub item in full collapsed mode
                SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                            0, (LPARAM)idChildObj);

                break;

              case OT_ICE_BUNIT_SEC_ROLE       :
              case OT_ICE_BUNIT_SEC_USER       :
                // 2 parents, no sub branch
                lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                              aparentsTemp[0],
                                              aparentsTemp[1],
                                              NULL,
                                              DBTYPE_REGULAR,
                                              buf, bufOwner, bufComplim, NULL);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                idChildObj = TreeAddRecord(lpDomData, buf, idAnchorObj,
                                           idInsertAfter, idInsertBefore, lpRecord);
                if (idChildObj==0)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                break;

              default:
                return MergeResStatus(iretGlob, RES_ERR);   // Fatal
            }

            // "Advance" for the next FindImmediateSubItem (sort assumed)
            idFirstOldChildObj = idChildObj;
          }

          // Manage the "Grayed" potential state for system objects :
          // we suppress the "expandability" feature
          // 20/9/95 : INCLUDING AT EXPANDITEM TIME
          // old code: if ( bOldCode || iAction != ACT_EXPANDITEM) {
          if ( bOldCode || 1 ) {
            if (!bwithsystem) {
              if (CanBeSystemObj(iobjecttype)) {
                lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                          LM_GETITEMDATA, 0, idChildObj);
                if (!lpRecord)
                  return MergeResStatus(iretGlob, RES_ERR);   // Fatal
                basicType = GetBasicType(iobjecttype);
                if (IsSystemObject2(basicType,
                                    lpRecord->objName,
                                    lpRecord->ownerName,
                                    lpRecord->parentDbType)) {
                  TreeDeleteAllChildren(lpDomData, idChildObj);
                  DumListRemove(idChildObj);  // in case delayed child
                }
              }
            }
          }

loopcontinue:

          // NO MATTER WHETHER RELATED OR NOT :
          // anchor aparentsResult on buffers again,
          // since we may have set aparentsResult[1] or [2] to NULL...
          aparentsResult[0] = bufParResult0;
          aparentsResult[1] = bufParResult1;
          aparentsResult[2] = bufParResult2;
          CleanStringsArray(aparentsResult,
                         sizeof(aparentsResult)/sizeof(aparentsResult[0]));

          // Get next object in the dom
          if (IsRelatedType(iobjecttype))
            iret = DOMGetNextRelObject(aparentsResult,
                                       buf, bufOwner, bufComplim);
          else
            iret = DOMGetNextObject(buf, bufOwner, bufComplim);
        }

        time3 = time(NULL);

        // 5 minus epsilon) create dummy sub-items
        if (!DumListUseAndFree(lpDomData))
          return MergeResStatus(iretGlob, RES_ERR);   // Fatal

        // 5) Delete remaining children marked for deletion
        TreeDeleteAllMarkedChildren(lpDomData, idAnchorObj,
                                               idLastOldChildObj);

        // 6) if no item, simply add item : < no >
        if (bNoItem) {
          LoadString(hResource, ids_none, bufRs, sizeof(bufRs));
          // 16/2/95: allocate without the parents
          // ( used aparentsTemp[0] to 2 before that date )
          // 14/3/95 : take parenthood, owner and complimentary info
          // from the static parent
          lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                          LM_GETITEMDATA,
                                          0, (LPARAM)idAnchorObj);
          lpRecord = AllocAndFillRecord(iobjecttype, TRUE,
                                        lpRecord->extra,
                                        lpRecord->extra2,
                                        lpRecord->extra3,
                                        lpRecord->parentDbType,
                                        "",                   // obj name
                                        lpRecord->ownerName,
                                        lpRecord->szComplim,
                                        lpRecord->tableOwner);
          if (!lpRecord)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
          if (TreeAddRecord(lpDomData, bufRs, idAnchorObj,
                            0, 0, lpRecord) == 0)
            return MergeResStatus(iretGlob, RES_ERR);   // Fatal
        }

        // 7) mark item as "with sub branch valid"
        lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                              LM_GETITEMDATA,
                                              0, (LPARAM)idAnchorObj);
        lpRecord->bSubValid = TRUE;

        // 7bis) set top item
        SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)idTop);

        // loop on "for (idAnchorObj)", see step 2bis
        // label necessary since we need to continue 2 loops at a time
LoopGetAnchorId:
        ;
      }

      // 8) only after the loop on anchor objects :
      // update the dependants branches that might need to be updated
      // IMPORTANT NOTE : We only receive OT_ types, no OTR_ types
      // refer to dom2.c
      if (iAction==ACT_BKTASK       ||  // TO BE FINISHED: BKTASK QUESTIONABLE ?
          iAction==ACT_ADD_OBJECT   ||
          iAction==ACT_ALTER_OBJECT ||
          iAction==ACT_DROP_OBJECT  ) {
        switch (iobjecttype) {
          case OT_SYNONYM:
            // keep aparents[0] safe, it should not be null! (base name)
            // scan the 3 related types
            aparents[1] = NULL;   // Search for all tables
            iret = DOMUpdateDisplayData2(hnodestruct, OTR_TABLESYNONYM,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            aparents[1] = NULL;   // Search for all views
            iret = DOMUpdateDisplayData2(hnodestruct, OTR_VIEWSYNONYM,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            aparents[1] = aparents[2] = NULL;   // all indexes on all tables
            iret = DOMUpdateDisplayData2(hnodestruct, OTR_INDEXSYNONYM,
                                        3, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_GROUP:
            aparents[0] = NULL;   // Search for all users
            iret = DOMUpdateDisplayData2(hnodestruct, OTR_USERGROUP,
                                        1, NULL, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_S_ALARM_SELSUCCESS_USER:
          case OT_S_ALARM_SELFAILURE_USER:
          case OT_S_ALARM_DELSUCCESS_USER:
          case OT_S_ALARM_DELFAILURE_USER:
          case OT_S_ALARM_INSSUCCESS_USER:
          case OT_S_ALARM_INSFAILURE_USER:
          case OT_S_ALARM_UPDSUCCESS_USER:
          case OT_S_ALARM_UPDFAILURE_USER:
            break;

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

          case OT_DBGRANT_SEQCRN_USER:
          case OT_DBGRANT_SEQCRY_USER:

            break;

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

          case OT_PROCGRANT_EXEC_USER:

          case OT_DBEGRANT_RAISE_USER:
          case OT_DBEGRANT_REGTR_USER:

          case OT_SEQUGRANT_NEXT_USER :

            break;

          case OT_RULE:
            // update the list of rules executing a given procedure,
            // for all procedures in the database where the rule
            // is/was defined
            // aparents[0] assumed to contain the base name
            aparents[1] = NULL; // for all procedures
            iret = DOMUpdateDisplayData2(hnodestruct,
                                        OTR_PROC_RULE,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_VIEW:
            // update the list of view referencing a given table,
            // for all the tables in the database where the view
            // is/was defined
            // aparents[0] assumed to contain the base name
            aparents[1] = NULL; // for all tables
            iret = DOMUpdateDisplayData2(hnodestruct,
                                        OTR_TABLEVIEW,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_TABLE:
            // update the list of tables on a given location,
            // for all locations
            aparents[0] = aparents[1] = NULL; // for all locations
            iret = DOMUpdateDisplayData2(hnodestruct,
                                        OTR_LOCATIONTABLE,
                                        1, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_REPLIC_REGTABLE:
            aparents[1] = NULL;   // for all tables
            iret = DOMUpdateDisplayData2(hnodestruct,
                                        OTR_REPLIC_TABLE_CDDS,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            iret = DOMUpdateDisplayData2(hnodestruct,
                                        OTR_REPLIC_CDDS_TABLE,
                                        2, aparents, FALSE,
                                        ACT_UPDATE_DEPENDANT_BRANCHES,
                                        0L, hwndDOMDoc,
                                        forceRefreshMode);
            iretGlob = MergeResStatus(iretGlob, iret);
            break;

          case OT_USER:
            // Notify nodes window to update the users list if expanded
            UpdateNodeUsersList(GetVirtNodeName(hnodestruct));
            break;

          default:
            // OTHER CASES TO BE FINISHED!
            //MessageBox(hwndDOMDoc, "Dependant branches not managed yet",
            //           NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
            break;
        }
      }

      time4 = time(NULL);

      // Tree speed improvement check
      #ifdef DEBUG_AUDIT_TREE
      {
        char szDebug[300];
        wsprintf(szDebug,
                 "Call to DOMGetFirst\t: %ld seconds\n"
                 "Loop with DOMGetNext\t: %d minutes %d seconds - total %ld seconds\n"
                 "Remaining work\t\t: %ld seconds\n"
                 "Total time\t\t: %d minutes %d seconds - total %ld seconds",
                 time2-time1,
                 (int)( (time3-time2)/60L ), (int)( (time3-time2)%60L ),
                                             time3-time2,
                 time4-time3,
                 (int)( (time4-time1)/60L ), (int)( (time4-time1)%60L ),
                                             time4-time1);
        MessageBox(hwndDOMDoc, szDebug, "Debug Elapsed times",
                   MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      }
      #endif  // DEBUG_AUDIT_TREE

      // 9) only after the loop on anchor objects :
      // manage the sub-items if bWithChildren is set
      // USELESS - NOT MANAGED

      // 9) Well done, boy!
      return MergeResStatus(iretGlob, RES_SUCCESS);
      break;

    default:
      return MergeResStatus(iretGlob, RES_SUCCESS);
      break;
  }
  return iretGlob;
}

//
// Builds the caption line for a security
//
// received: buffer with the security name, size assumed to be BUFSIZE
//           security number
//
// returned: formatted caption in the buffer that contained the security name
//
#define MAXSPAN 4   // max width for integrity number
static VOID NEAR MakeSecurityCaption(UCHAR *buf, long secNo, UCHAR *alarmName)
{
  UCHAR bufTemp[BUFSIZE];
  UCHAR bufTemp2[BUFSIZE];
  int   len;
  int   lmargin;

  wsprintf(bufTemp2, "%ld", secNo);
  len = x_strlen(bufTemp2);
  if (len >= MAXSPAN)
    lmargin = 0;
  else
    lmargin = MAXSPAN - len;
  memset(bufTemp, ' ', lmargin);
  if (alarmName[0])
    wsprintf(bufTemp+lmargin, "(%s / %s) %s", bufTemp2, alarmName, buf);
  else 
    wsprintf(bufTemp+lmargin, "(%s) %s", bufTemp2, buf);

  fstrncpy(buf, bufTemp,BUFSIZE);
}

//
// Builds the caption line for a cross security
//
// received: buffer with the security name, size assumed to be BUFSIZE
//           buffer with the parent base name
//           security number
//
// returned: formatted caption in the buffer that contained the security name
//
static VOID NEAR MakeCrossSecurityCaption(UCHAR *buf, UCHAR *parent, long secNo)
{
  UCHAR bufTemp[BUFSIZE];
  UCHAR bufTemp2[BUFSIZE];
  int   len;
  int   lmargin;

  wsprintf(bufTemp2, "%ld", secNo);
  len = x_strlen(bufTemp2);
  if (len >= MAXSPAN)
    lmargin = 0;
  else
    lmargin = MAXSPAN - len;
  memset(bufTemp, ' ', lmargin);
  wsprintf(bufTemp+lmargin, "(%s) [%s] %s", bufTemp2, parent, buf);

  fstrncpy(buf, bufTemp,BUFSIZE);
}

//
// Builds the caption line for an object of level 1,
// such as table, view, dbevent, procedure
// so we display the parent base name in the caption
//
// received: buffer with the object name, size assumed to be BUFSIZE
//           buffer with the parent base name
//
// returned: formatted caption in the buffer that contained the object name
//
static VOID NEAR MakeRelatedLevel1ObjCaption(UCHAR *buf, UCHAR *parent)
{
  UCHAR bufTemp[BUFSIZE];

  wsprintf(bufTemp, "[%s] %s", parent, buf);
  fstrncpy(buf, bufTemp,BUFSIZE);
}

//
//  Merges the two RES_XXX received and returns the merge
//
//  At this time, returns the most severe error between both
//
//  RES_ENDOFDATA and RES_ALREADYEXIST not concerned with
//
//  Severity order (least to most) :
//    RES_SUCCESS
//    RES_NOGRANT
//    RES_TIMEOUT
//    RES_ERR
//
static int  NEAR MergeResStatus(int iret1, int iret2)
{
  switch (iret1) {
    case RES_SUCCESS:
      return iret2;
      break;
    case RES_NOGRANT:
      if (iret2 == RES_SUCCESS)
        return iret1;
      else
        return iret2;
    case RES_TIMEOUT:
      if (iret2 == RES_SUCCESS || iret2 == RES_NOGRANT)
        return iret1;
      else
        return iret2;
    case RES_ERR:
      return RES_ERR;
    default:
      return iret2;
  }
}

//
// Adds a static sub item, itself having optionnaly a dummy sub item
//
// parameters:
//  LPDOMDATA lpDomData     : guess what
//  DWORD     idParent      : id of the parent branch for the static sub item
//  UCHAR    *bufParent1    : parenthood string 1 for object in parent branch
//  UCHAR    *bufParent2    : parenthood string 2 for object in parent branch
//  UCHAR    *bufParent3    : parenthood string 3 for object in parent branch
//  int       parentDbType  : parent database type (DBTYPE_REGULAR, DBTYPE_DISTRIBUTED, DBTYPE_COORDINATOR)
//  UCHAR    *bufOwner      : owner of the parent branch object
//  UCHAR    *bufComplim    : complimentary data of the parent branch object
//  UCHAR    *tableOwner    : if parent of level 1 has a schema, it's schema
//  int       otStatic      : object type of the sub item to be created
//  UINT      idRsString    : text resource id of the sub item to be created
//  BOOL      bWithDummy    : Should the sub item have a dummy sub-item?
//     added 25/8/95 :
//  BOOL      bSpeedFlag    : TRUE if we want fast but not full collapse mode,
//                            FALSE if we want slow but full collapse mode.
//                            Significant only if bWithDummy is set
//                            The caller MUST have this function called
//                            with FALSE for the last call, or must be sure
//                            a LM_COLLAPSE with 0 in wParam will be sent.
//
static DWORD AddStaticSubItem(LPDOMDATA lpDomData, DWORD idParent, UCHAR *bufParent1, UCHAR *bufParent2, UCHAR *bufParent3, int parentDbType, UCHAR *bufOwner, UCHAR *bufComplim, UCHAR *tableOwner, int otStatic, UINT idRsString, BOOL bWithDummy, BOOL bSpeedFlag)
{
  UCHAR   bufRs[BUFSIZE];       // resource string
  DWORD   idChild;
  LPTREERECORD  lpRecord;

  LoadString(hResource, idRsString, bufRs, sizeof(bufRs));

  // allocate with subvalid set to TRUE if no dummy sub-item,
  // otherwise allocate with subvalid set to FALSE
  lpRecord = AllocAndFillRecord(otStatic, !(bWithDummy),
                                bufParent1, bufParent2, bufParent3,
                                parentDbType,
                                bufRs, bufOwner, bufComplim, tableOwner);
  if (!lpRecord)
    return 0L;    // fatal
  idChild = TreeAddRecord(lpDomData, bufRs, idParent, 0, 0, lpRecord);
  if (idChild==0)
    return 0L;    // fatal

  // create dummy sub-item if required
  if (bWithDummy) {
    if (AddDummySubItem(lpDomData, idChild)==0) {
      // force full collapse mode to get the tree recalculated
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)idChild);
      // return fatal error code
      return 0L;
    }

    // Collapse parent
    SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                ( bSpeedFlag ? 0xdcba : 0),
                (LPARAM)idChild);
  }

  // return record id
  return idChild;
}

//
//  Says whether the object type is a "related type"
//  (i.e. OTR_xxx)
//
BOOL IsRelatedType(int iobjecttype)
{
  switch (iobjecttype) {
    case OTR_USERSCHEMA:
    case OTR_USERGROUP:
    case OTR_ALARM_SELSUCCESS_TABLE:
    case OTR_ALARM_SELFAILURE_TABLE:
    case OTR_ALARM_DELSUCCESS_TABLE:
    case OTR_ALARM_DELFAILURE_TABLE:
    case OTR_ALARM_INSSUCCESS_TABLE:
    case OTR_ALARM_INSFAILURE_TABLE:
    case OTR_ALARM_UPDSUCCESS_TABLE:
    case OTR_ALARM_UPDFAILURE_TABLE:

    case OTR_GRANTEE_RAISE_DBEVENT:
    case OTR_GRANTEE_REGTR_DBEVENT:
    case OTR_GRANTEE_EXEC_PROC:
    case OTR_GRANTEE_NEXT_SEQU:
    case OTR_DBGRANT_ACCESY_DB:
    case OTR_DBGRANT_ACCESN_DB:
    case OTR_DBGRANT_CREPRY_DB:
    case OTR_DBGRANT_CREPRN_DB:
    case OTR_DBGRANT_SEQCRY_DB:
    case OTR_DBGRANT_SEQCRN_DB:
    case OTR_DBGRANT_CRETBY_DB:
    case OTR_DBGRANT_CRETBN_DB:
    case OTR_DBGRANT_DBADMY_DB:
    case OTR_DBGRANT_DBADMN_DB:
    case OTR_DBGRANT_LKMODY_DB:
    case OTR_DBGRANT_LKMODN_DB:
    case OTR_DBGRANT_QRYIOY_DB:
    case OTR_DBGRANT_QRYION_DB:
    case OTR_DBGRANT_QRYRWY_DB:
    case OTR_DBGRANT_QRYRWN_DB:
    case OTR_DBGRANT_UPDSCY_DB:
    case OTR_DBGRANT_UPDSCN_DB:

    case OTR_DBGRANT_SELSCY_DB:
    case OTR_DBGRANT_SELSCN_DB:
    case OTR_DBGRANT_CNCTLY_DB:
    case OTR_DBGRANT_CNCTLN_DB:
    case OTR_DBGRANT_IDLTLY_DB:
    case OTR_DBGRANT_IDLTLN_DB:
    case OTR_DBGRANT_SESPRY_DB:
    case OTR_DBGRANT_SESPRN_DB:
    case OTR_DBGRANT_TBLSTY_DB:
    case OTR_DBGRANT_TBLSTN_DB:

    case OTR_DBGRANT_QRYCPY_DB:
    case OTR_DBGRANT_QRYCPN_DB:
    case OTR_DBGRANT_QRYPGY_DB:
    case OTR_DBGRANT_QRYPGN_DB:
    case OTR_DBGRANT_QRYCOY_DB:
    case OTR_DBGRANT_QRYCON_DB:

    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_CPI_TABLE:
    case OTR_GRANTEE_CPF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
    case OTR_GRANTEE_SEL_VIEW:
    case OTR_GRANTEE_INS_VIEW:
    case OTR_GRANTEE_UPD_VIEW:
    case OTR_GRANTEE_DEL_VIEW:
    case OTR_GRANTEE_ROLE:

    case OTR_TABLESYNONYM:
    case OTR_TABLEVIEW:
    case OTR_VIEWSYNONYM:
    case OTR_INDEXSYNONYM:
    case OTR_LOCATIONTABLE:

    case OTR_PROC_RULE:

    case OTR_REPLIC_CDDS_TABLE:

    case OTR_REPLIC_TABLE_CDDS:

    case OTR_CDB:

      return TRUE;

    default:
      return FALSE;
  }
  return FALSE;
}

//
//  Says whether we can have an object of the requested type that will be
//  a system object, even though the "show system objects" flag is not true
//  Example 1: list of users in a group ---> we could have $ingres
//  Example 2: list of locations of a table ---> iidatabase
//
// defaults to FALSE
//
BOOL CanBeSystemObj(int iobjecttype)
{
  switch (iobjecttype) {
    case OT_SCHEMAUSER:       // schemas on a database
    case OT_GROUPUSER:        // Users in a group
    case OT_TABLELOCATION:    // Locations on which a table is
    case OT_ROLEGRANT_USER:   // user with grant on role

    // Alarmees on table
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:

    // Alarmees on database
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:

    // Unsolved grantees - QUESTIONABLE SINCE CAN BE USER, GROUP OR ROLE
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

    case OT_DBGRANT_SEQCRN_USER:
    case OT_DBGRANT_SEQCRY_USER:

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

    // Solved grantees
    case OT_GRANTEE_SOLVED_USER:
    case OT_GRANTEE_SOLVED_GROUP:
    case OT_GRANTEE_SOLVED_ROLE:

    // new style alarms
    case OT_ALARMEE:
    //case OT_ALARMEE_SOLVED_USER:
    //case OT_ALARMEE_SOLVED_GROUP:
    //case OT_ALARMEE_SOLVED_ROLE:

    // added Jan 20, 97
    case OT_VIEWTABLE:

    // added Sept 10, 97:
    case OT_RULEPROC:

      return TRUE;

    default:
      return FALSE;
  }
  return FALSE;     // Caution
}

//
//  Returns the corresponding basic type for the received type
//
//  Example: OT_GROUPUSER ---> OT_USER, OT_TABLELOCATION ---> OT_LOCATION
//
// MUST be called before calling IsSystemObject, HasSystemObjects,
// HasExtraDisplayString or GetExtraDisplayStringCaption
//
int GetBasicType(int iobjecttype)
{
  switch (iobjecttype) {
    // MASQUED OUT SINCE CUSTOM MANAGEMENT: case OT_SCHEMAUSER:       // schemas on a database

    case OT_GROUPUSER:        // Users in a group
    case OT_ROLEGRANT_USER:   // user with grant on role
      return OT_USER;

    case OT_TABLELOCATION:    // Locations on which a table is
      return OT_LOCATION;

    case OTR_TABLESYNONYM:
    case OTR_VIEWSYNONYM:
    case OTR_INDEXSYNONYM:
      return OT_SYNONYM;

    case OTR_USERGROUP:
      return OT_GROUP;

    // view components : can be table or view. system criterion is the same
    case OT_VIEWTABLE:
      return OT_TABLE;

    // alarms : launched dbevent
    case OT_S_ALARM_EVENT:
      return OT_DBEVENT;

    // Alarmees on table and database
    //  - QUESTIONABLE SINCE CAN BE USER, GROUP OR ROLE
    case OT_ALARMEE:
      return OT_USER;

    // Solved alarmees
    //case OT_ALARMEE_SOLVED_USER:
    //  return OT_USER;
    //case OT_ALARMEE_SOLVED_GROUP:
    //  return OT_GROUP;
    //case OT_ALARMEE_SOLVED_ROLE:
    //  return OT_ROLE;

    // Grantees - QUESTIONABLE SINCE CAN BE USER, GROUP OR ROLE
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

    case OT_DBGRANT_SEQCRN_USER:
    case OT_DBGRANT_SEQCRY_USER:

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

      // Grantees: no type solve if desktop, so that no right pane
      return OT_USER;

    // Solved grantees
    case OT_GRANTEE_SOLVED_USER:
      return OT_USER;
    case OT_GRANTEE_SOLVED_GROUP:
      return OT_GROUP;
    case OT_GRANTEE_SOLVED_ROLE:
      return OT_ROLE;

    // Same as database
    case OTR_USERSCHEMA:
    case OTR_DBGRANT_ACCESY_DB:
    case OTR_DBGRANT_ACCESN_DB:
    case OTR_DBGRANT_CREPRY_DB:
    case OTR_DBGRANT_CREPRN_DB:
    case OTR_DBGRANT_SEQCRY_DB:
    case OTR_DBGRANT_SEQCRN_DB:
    case OTR_DBGRANT_CRETBY_DB:
    case OTR_DBGRANT_CRETBN_DB:
    case OTR_DBGRANT_DBADMY_DB:
    case OTR_DBGRANT_DBADMN_DB:
    case OTR_DBGRANT_LKMODY_DB:
    case OTR_DBGRANT_LKMODN_DB:
    case OTR_DBGRANT_QRYIOY_DB:
    case OTR_DBGRANT_QRYION_DB:
    case OTR_DBGRANT_QRYRWY_DB:
    case OTR_DBGRANT_QRYRWN_DB:
    case OTR_DBGRANT_UPDSCY_DB:
    case OTR_DBGRANT_UPDSCN_DB:

    case OTR_DBGRANT_SELSCY_DB:
    case OTR_DBGRANT_SELSCN_DB:
    case OTR_DBGRANT_CNCTLY_DB:
    case OTR_DBGRANT_CNCTLN_DB:
    case OTR_DBGRANT_IDLTLY_DB:
    case OTR_DBGRANT_IDLTLN_DB:
    case OTR_DBGRANT_SESPRY_DB:
    case OTR_DBGRANT_SESPRN_DB:
    case OTR_DBGRANT_TBLSTY_DB:
    case OTR_DBGRANT_TBLSTN_DB:

    case OTR_DBGRANT_QRYCPY_DB:
    case OTR_DBGRANT_QRYCPN_DB:
    case OTR_DBGRANT_QRYPGY_DB:
    case OTR_DBGRANT_QRYPGN_DB:
    case OTR_DBGRANT_QRYCOY_DB:
    case OTR_DBGRANT_QRYCON_DB:

      return OT_DATABASE;

    // same as procedure
    case OT_RULEPROC:
    case OTR_GRANTEE_EXEC_PROC:
      return OT_PROCEDURE;

    case OTR_GRANTEE_NEXT_SEQU:
      return OT_SEQUENCE;

    // same as dbevent
    case OTR_GRANTEE_RAISE_DBEVENT:
    case OTR_GRANTEE_REGTR_DBEVENT:
      return OT_DBEVENT;

    // same as view
    case OTR_TABLEVIEW:
    case OTR_GRANTEE_SEL_VIEW:
    case OTR_GRANTEE_INS_VIEW:
    case OTR_GRANTEE_UPD_VIEW:
    case OTR_GRANTEE_DEL_VIEW:
      return OT_VIEW;

    // same as table
    case OTR_LOCATIONTABLE:
    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_CPI_TABLE:
    case OTR_GRANTEE_CPF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
      return OT_TABLE;

     // same as role
    case OTR_GRANTEE_ROLE:
      return OT_ROLE;

    // replicator
    case OTR_REPLIC_CDDS_TABLE:
      return OT_TABLE;

    case OTR_REPLIC_TABLE_CDDS:
      return OT_REPLIC_CDDS;

    // Emb 02/02/98 : for right pane, a replicated table is a table
    case OT_REPLIC_REGTABLE:
      return OT_TABLE;

    // Emb 16/02/98 : for right pane, the cdb of a ddb is a database
    case OTR_CDB:
      return OT_DATABASE;

    default:
      return iobjecttype;
  }
  return 0;     // We missed something in the switch...
}

//
//  set every string of an array of strings to empty
//  needs to be used before calling domdata functions that do not
//  do the cleanup themselves when they don't return valid data in
//  elements of the array
//
static VOID NEAR CleanStringsArray(LPUCHAR *array, int stringsNumber)
{
  int cpt;

  for (cpt=0; cpt<stringsNumber; cpt++)
    array[cpt][0] = '\0';
}

//
//  returns the object type of the children that would be shown
//  when we expand an item of the received type
//  The received type may be static or not
//  The returned type will be a non-static object type,
//  or OT_STATIC_DUMMY if the branch expansion would display sub-statics
//
//  IMPORTANT NOTE : this function needs to be updated for every new type
//
//  if NoSolve, returns OTLL_xxx instead of OT_xxx_SOLVE,
//  for force refresh purpose
//
int GetChildType(int iobjecttype, BOOL bNoSolve)
{
  switch (iobjecttype) {

    // level 0
    case OT_STATIC_DATABASE:
      return OT_DATABASE;
      break;

    case OT_DATABASE:
      // expansion to create static sub-items
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_PROFILE:
      return OT_PROFILE;
      break;

    case OT_STATIC_USER:
      return OT_USER;
      break;

    case OT_STATIC_GROUP:
      return OT_GROUP;
      break;

    case OT_STATIC_ROLE:
      return OT_ROLE;
      break;

    case OT_STATIC_LOCATION:
      return OT_LOCATION;
      break;

    // pseudo level 0 - to determine the type of object
    // lying under a grantee
    // i.e: is the grantee a user, a group, or a role
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
    case OT_DBEGRANT_RAISE_USER:  // added 14/3/95
    case OT_DBEGRANT_REGTR_USER:  // added 14/3/95
    case OT_PROCGRANT_EXEC_USER:  // added 14/3/95
    case OT_SEQUGRANT_NEXT_USER:
      if (bNoSolve)
        return OTLL_GRANTEE;
      else
        return OT_GRANTEE_SOLVE;
      break;

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
    case OT_DBGRANT_SEQCRN_USER:
    case OT_DBGRANT_SEQCRY_USER:
      if (bNoSolve)
        return OTLL_DBGRANTEE;
      else
        return OT_GRANTEE_SOLVE;
      break;

    case OT_USER:
    case OT_GROUPUSER:
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_S_ALARM_UPDFAILURE_USER:
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:
      // expansion to create static sub-items
      return OT_STATIC_DUMMY;
      break;

    // branches to reach the synonymed
    case OT_SYNONYM:
    case OTR_TABLESYNONYM:
    case OTR_VIEWSYNONYM:
    case OTR_INDEXSYNONYM:
    case OT_STATIC_SYNONYMED:
      return OT_STATIC_DUMMY;
      break;

    // pseudo level 1 : solve the "synonymed" type
    case OT_SYNONYMOBJECT:
      if (bNoSolve)
        return OT_SYNONYMOBJECT;
      else 
        return OT_SYNONYMED_SOLVE;     // TO BE FINISHED : 3 TYPES SOLVE
      break;

    // level 1, child of database
    case OT_STATIC_R_CDB:
      return OTR_CDB;
    case OTR_CDB:
      return OT_STATIC_DUMMY;   // expansion to create static sub-items

    case OT_STATIC_TABLE:
      return OT_TABLE;
      break;

    case OT_STATIC_SCHEMAUSER_TABLE:
      return OT_SCHEMAUSER_TABLE;
      break;

    case OT_TABLE:
    case OTR_ALARM_SELSUCCESS_TABLE:
    case OTR_ALARM_SELFAILURE_TABLE:
    case OTR_ALARM_DELSUCCESS_TABLE:
    case OTR_ALARM_DELFAILURE_TABLE:
    case OTR_ALARM_INSSUCCESS_TABLE:
    case OTR_ALARM_INSFAILURE_TABLE:
    case OTR_ALARM_UPDSUCCESS_TABLE:
    case OTR_ALARM_UPDFAILURE_TABLE:
    case OT_SCHEMAUSER_TABLE:
      // expansion to create static sub-items
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_VIEW:
      return OT_VIEW;
      break;

    case OT_STATIC_SCHEMAUSER_VIEW:
      return OT_SCHEMAUSER_VIEW;
      break;

    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      // expansion to create static sub-items
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_PROCEDURE:
      return OT_PROCEDURE;
      break;

    case OT_STATIC_SEQUENCE:
      return OT_SEQUENCE;
      break;

    case OT_STATIC_SCHEMAUSER_PROCEDURE:
      return OT_SCHEMAUSER_PROCEDURE;
      break;

    case OT_STATIC_SCHEMA:
      return OT_SCHEMAUSER;
      break;

    case OT_STATIC_SYNONYM:
      return OT_SYNONYM;
      break;

    case OT_STATIC_DBGRANTEES:
      // expansion to create static sub-items
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_DBGRANTEES_ACCESY:
      return OT_DBGRANT_ACCESY_USER;
      break;

    case OT_STATIC_DBGRANTEES_ACCESN:
      return OT_DBGRANT_ACCESN_USER;
      break;

    case OT_STATIC_DBGRANTEES_CREPRY:
      return OT_DBGRANT_CREPRY_USER;
      break;

    case OT_STATIC_DBGRANTEES_CREPRN:
      return OT_DBGRANT_CREPRN_USER;
      break;

    case OT_STATIC_DBGRANTEES_CRETBY:
      return OT_DBGRANT_CRETBY_USER;
      break;

    case OT_STATIC_DBGRANTEES_CRETBN:
      return OT_DBGRANT_CRETBN_USER;
      break;

    case OT_STATIC_DBGRANTEES_DBADMY:
      return OT_DBGRANT_DBADMY_USER;
      break;

    case OT_STATIC_DBGRANTEES_DBADMN:
      return OT_DBGRANT_DBADMN_USER;
      break;

    case OT_STATIC_DBGRANTEES_LKMODY:
      return OT_DBGRANT_LKMODY_USER;
      break;

    case OT_STATIC_DBGRANTEES_LKMODN:
      return OT_DBGRANT_LKMODN_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYIOY:
      return OT_DBGRANT_QRYIOY_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYION:
      return OT_DBGRANT_QRYION_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYRWY:
      return OT_DBGRANT_QRYRWY_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYRWN:
      return OT_DBGRANT_QRYRWN_USER;
      break;

    case OT_STATIC_DBGRANTEES_UPDSCY:
      return OT_DBGRANT_UPDSCY_USER;
      break;

    case OT_STATIC_DBGRANTEES_UPDSCN:
      return OT_DBGRANT_UPDSCN_USER;
      break;

    case OT_STATIC_DBGRANTEES_SELSCY:
      return OT_DBGRANT_SELSCY_USER;
      break;

    case OT_STATIC_DBGRANTEES_SELSCN:
      return OT_DBGRANT_SELSCN_USER;
      break;

    case OT_STATIC_DBGRANTEES_CNCTLY:
      return OT_DBGRANT_CNCTLY_USER;
      break;

    case OT_STATIC_DBGRANTEES_CNCTLN:
      return OT_DBGRANT_CNCTLN_USER;
      break;

    case OT_STATIC_DBGRANTEES_IDLTLY:
      return OT_DBGRANT_IDLTLY_USER;
      break;

    case OT_STATIC_DBGRANTEES_IDLTLN:
      return OT_DBGRANT_IDLTLN_USER;
      break;

    case OT_STATIC_DBGRANTEES_SESPRY:
      return OT_DBGRANT_SESPRY_USER;
      break;

    case OT_STATIC_DBGRANTEES_SESPRN:
      return OT_DBGRANT_SESPRN_USER;
      break;

    case OT_STATIC_DBGRANTEES_TBLSTY:
      return OT_DBGRANT_TBLSTY_USER;
      break;
    case OT_STATIC_DBGRANTEES_TBLSTN:
      return OT_DBGRANT_TBLSTN_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYCPY:
      return OT_DBGRANT_QRYCPY_USER;
      break;
    case OT_STATIC_DBGRANTEES_QRYCPN:
      return OT_DBGRANT_QRYCPN_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYPGY:
      return OT_DBGRANT_QRYPGY_USER;
      break;
    case OT_STATIC_DBGRANTEES_QRYPGN:
      return OT_DBGRANT_QRYPGN_USER;
      break;

    case OT_STATIC_DBGRANTEES_QRYCOY:
      return OT_DBGRANT_QRYCOY_USER;
      break;
    case OT_STATIC_DBGRANTEES_QRYCON:
      return OT_DBGRANT_QRYCON_USER;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQY:
      return OT_DBGRANT_SEQCRY_USER;
      break;
    case OT_STATIC_DBGRANTEES_CRSEQN:
      return OT_DBGRANT_SEQCRN_USER;
      break;

    case OT_STATIC_DBEVENT:
      return OT_DBEVENT;
      break;

    case OT_STATIC_DBALARM:
      return OT_STATIC_DUMMY; // sub-statics
      break;
    case OT_STATIC_DBALARM_CONN_SUCCESS:
      return OT_S_ALARM_CO_SUCCESS_USER;
      break;
    case OT_STATIC_DBALARM_CONN_FAILURE:
      return OT_S_ALARM_CO_FAILURE_USER;
      break;
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
      return OT_S_ALARM_DI_SUCCESS_USER;
      break;
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
      return OT_S_ALARM_DI_FAILURE_USER;
      break;

    // level 1, child of group
    case OT_STATIC_GROUPUSER:
      return OT_GROUPUSER;
      break;

    // level 1, child of user or group or role
    case OT_STATIC_R_GRANT:
      return OT_STATIC_DUMMY;
      // sub-statics return OTR_GRANTS_SUBSTATICS;
      break;

    case OT_STATIC_R_DBEGRANT:
    case OT_STATIC_R_PROCGRANT:
    case OT_STATIC_R_DBGRANT:
    case OT_STATIC_R_TABLEGRANT:
    case OT_STATIC_R_VIEWGRANT:
    case OT_STATIC_R_SEQGRANT:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_R_DBEGRANT_RAISE:
      return OTR_GRANTEE_RAISE_DBEVENT;
      break;

    case OT_STATIC_R_DBEGRANT_REGISTER:
      return OTR_GRANTEE_REGTR_DBEVENT;
      break;

    case OT_STATIC_R_PROCGRANT_EXEC:
      return OTR_GRANTEE_EXEC_PROC;
      break;

    case OT_STATIC_R_SEQGRANT_NEXT:
      return OTR_GRANTEE_NEXT_SEQU;
      break;

    case OT_STATIC_R_DBGRANT_ACCESY:
      return OTR_DBGRANT_ACCESY_DB;
      break;

    case OT_STATIC_R_DBGRANT_ACCESN:
      return OTR_DBGRANT_ACCESN_DB;
      break;

    case OT_STATIC_R_DBGRANT_CREPRY:
      return OTR_DBGRANT_CREPRY_DB;
      break;

    case OT_STATIC_R_DBGRANT_CREPRN:
      return OTR_DBGRANT_CREPRN_DB;
      break;

    case OT_STATIC_R_DBGRANT_CRETBY:
      return OTR_DBGRANT_CRETBY_DB;
      break;

    case OT_STATIC_R_DBGRANT_CRETBN:
      return OTR_DBGRANT_CRETBN_DB;
      break;

    case OT_STATIC_R_DBGRANT_DBADMY:
      return OTR_DBGRANT_DBADMY_DB;
      break;

    case OT_STATIC_R_DBGRANT_DBADMN:
      return OTR_DBGRANT_DBADMN_DB;
      break;

    case OT_STATIC_R_DBGRANT_LKMODY:
      return OTR_DBGRANT_LKMODY_DB;
      break;

    case OT_STATIC_R_DBGRANT_LKMODN:
      return OTR_DBGRANT_LKMODN_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYIOY:
      return OTR_DBGRANT_QRYIOY_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYION:
      return OTR_DBGRANT_QRYION_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYRWY:
      return OTR_DBGRANT_QRYRWY_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYRWN:
      return OTR_DBGRANT_QRYRWN_DB;
      break;

    case OT_STATIC_R_DBGRANT_UPDSCY:
      return OTR_DBGRANT_UPDSCY_DB;
      break;

    case OT_STATIC_R_DBGRANT_UPDSCN:
      return OTR_DBGRANT_UPDSCN_DB;
      break;

    case OT_STATIC_R_DBGRANT_SELSCY:
      return OTR_DBGRANT_SELSCY_DB;
      break;

    case OT_STATIC_R_DBGRANT_SELSCN:
      return OTR_DBGRANT_SELSCN_DB;
      break;

    case OT_STATIC_R_DBGRANT_CNCTLY:
      return OTR_DBGRANT_CNCTLY_DB;
      break;

    case OT_STATIC_R_DBGRANT_CNCTLN:
      return OTR_DBGRANT_CNCTLN_DB;
      break;

    case OT_STATIC_R_DBGRANT_IDLTLY:
      return OTR_DBGRANT_IDLTLY_DB;
      break;

    case OT_STATIC_R_DBGRANT_IDLTLN:
      return OTR_DBGRANT_IDLTLN_DB;
      break;

    case OT_STATIC_R_DBGRANT_SESPRY:
      return OTR_DBGRANT_SESPRY_DB;
      break;

    case OT_STATIC_R_DBGRANT_SESPRN:
      return OTR_DBGRANT_SESPRN_DB;
      break;

    case OT_STATIC_R_DBGRANT_TBLSTY:
      return OTR_DBGRANT_TBLSTY_DB;
      break;
    case OT_STATIC_R_DBGRANT_TBLSTN:
      return OTR_DBGRANT_TBLSTN_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYCPY:
      return OTR_DBGRANT_QRYCPY_DB;
      break;
    case OT_STATIC_R_DBGRANT_QRYCPN:
      return OTR_DBGRANT_QRYCPN_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYPGY:
      return OTR_DBGRANT_QRYPGY_DB;
      break;
    case OT_STATIC_R_DBGRANT_QRYPGN:
      return OTR_DBGRANT_QRYPGN_DB;
      break;

    case OT_STATIC_R_DBGRANT_QRYCOY:
      return OTR_DBGRANT_QRYCOY_DB;
      break;
    case OT_STATIC_R_DBGRANT_QRYCON:
      return OTR_DBGRANT_QRYCON_DB;
      break;

    case OT_STATIC_R_DBGRANT_CRESEQY:
      return OTR_DBGRANT_SEQCRY_DB;
      break;
    case OT_STATIC_R_DBGRANT_CRESEQN:
      return OTR_DBGRANT_SEQCRN_DB;
      break;

    case OT_STATIC_R_TABLEGRANT_SEL:
      return OTR_GRANTEE_SEL_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_INS:
      return OTR_GRANTEE_INS_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_UPD:
      return OTR_GRANTEE_UPD_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_DEL:
      return OTR_GRANTEE_DEL_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_REF:
      return OTR_GRANTEE_REF_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_CPI:
      return OTR_GRANTEE_CPI_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_CPF:
      return OTR_GRANTEE_CPF_TABLE;
      break;

    case OT_STATIC_R_TABLEGRANT_ALL:
      return OTR_GRANTEE_ALL_TABLE;
      break;

    case OT_STATIC_R_VIEWGRANT_SEL:
      return OTR_GRANTEE_SEL_VIEW;
      break;

    case OT_STATIC_R_VIEWGRANT_INS:
      return OTR_GRANTEE_INS_VIEW;
      break;

    case OT_STATIC_R_VIEWGRANT_UPD:
      return OTR_GRANTEE_UPD_VIEW;
      break;

    case OT_STATIC_R_VIEWGRANT_DEL:
      return OTR_GRANTEE_DEL_VIEW;
      break;

    case OT_STATIC_R_ROLEGRANT:
      return OTR_GRANTEE_ROLE;
      break;

    case OT_STATIC_R_USERSCHEMA:
      return OTR_USERSCHEMA;
      break;

    case OT_STATIC_R_USERGROUP:
      return OTR_USERGROUP;
      break;

    case OT_STATIC_R_SECURITY:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_R_SEC_SEL_SUCC:
      return OTR_ALARM_SELSUCCESS_TABLE;
      break;

    case OT_STATIC_R_SEC_SEL_FAIL:
      return OTR_ALARM_SELFAILURE_TABLE;
      break;

    case OT_STATIC_R_SEC_DEL_SUCC:
      return OTR_ALARM_DELSUCCESS_TABLE;
      break;

    case OT_STATIC_R_SEC_DEL_FAIL:
      return OTR_ALARM_DELFAILURE_TABLE;
      break;

    case OT_STATIC_R_SEC_INS_SUCC:
      return OTR_ALARM_INSSUCCESS_TABLE;
      break;

    case OT_STATIC_R_SEC_INS_FAIL:
      return OTR_ALARM_INSFAILURE_TABLE;
      break;

    case OT_STATIC_R_SEC_UPD_SUCC:
      return OTR_ALARM_UPDSUCCESS_TABLE;
      break;

    case OT_STATIC_R_SEC_UPD_FAIL:
      return OTR_ALARM_UPDFAILURE_TABLE;
      break;

    // level 1, child of role
    case OT_STATIC_ROLEGRANT_USER:
      return OT_ROLEGRANT_USER;
      break;

    // level 2, child of "table of database"
    case OT_STATIC_INTEGRITY:
      return OT_INTEGRITY;
      break;

    case OT_STATIC_RULE:
      return OT_RULE;
      break;

    case OT_STATIC_INDEX:
      return OT_INDEX;
      break;

    case OT_STATIC_TABLEGRANTEES:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_TABLEGRANT_SEL_USER:
      return OT_TABLEGRANT_SEL_USER;
      break;

    case OT_STATIC_TABLEGRANT_INS_USER:
      return OT_TABLEGRANT_INS_USER;
      break;

    case OT_STATIC_TABLEGRANT_UPD_USER:
      return OT_TABLEGRANT_UPD_USER;
      break;

    case OT_STATIC_TABLEGRANT_DEL_USER:
      return OT_TABLEGRANT_DEL_USER;
      break;

    case OT_STATIC_TABLEGRANT_REF_USER:
      return OT_TABLEGRANT_REF_USER;
      break;

    case OT_STATIC_TABLEGRANT_CPI_USER:
      return OT_TABLEGRANT_CPI_USER;
      break;

    case OT_STATIC_TABLEGRANT_CPF_USER:
      return OT_TABLEGRANT_CPF_USER;
      break;

    case OT_STATIC_TABLEGRANT_ALL_USER:
      return OT_TABLEGRANT_ALL_USER;
      break;

    // desktop
    case OT_STATIC_TABLEGRANT_INDEX_USER:
      return OT_TABLEGRANT_INDEX_USER;
      break;

    // desktop
    case OT_STATIC_TABLEGRANT_ALTER_USER:
      return OT_TABLEGRANT_ALTER_USER;
      break;

    case OT_STATIC_TABLELOCATION:
      return OT_TABLELOCATION;
      break;

    case OT_STATIC_SECURITY:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_SEC_SEL_SUCC:
      return OT_S_ALARM_SELSUCCESS_USER;
      break;

    case OT_STATIC_SEC_SEL_FAIL:
      return OT_S_ALARM_SELFAILURE_USER;
      break;

    case OT_STATIC_SEC_DEL_SUCC:
      return OT_S_ALARM_DELSUCCESS_USER;
      break;

    case OT_STATIC_SEC_DEL_FAIL:
      return OT_S_ALARM_DELFAILURE_USER;
      break;

    case OT_STATIC_SEC_INS_SUCC:
      return OT_S_ALARM_INSSUCCESS_USER;
      break;

    case OT_STATIC_SEC_INS_FAIL:
      return OT_S_ALARM_INSFAILURE_USER;
      break;

    case OT_STATIC_SEC_UPD_SUCC:
      return OT_S_ALARM_UPDSUCCESS_USER;
      break;

    case OT_STATIC_SEC_UPD_FAIL:
      return OT_S_ALARM_UPDFAILURE_USER;
      break;

    case OT_STATIC_R_TABLESYNONYM:
      return OTR_TABLESYNONYM;
      break;

    case OT_STATIC_R_TABLEVIEW:
      return OTR_TABLEVIEW;
      break;

    // level 2, child of "view of database"
    case OT_STATIC_VIEWTABLE:
      return OT_VIEWTABLE;
      break;

    case OT_STATIC_VIEWGRANTEES:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_VIEWGRANT_SEL_USER:
      return OT_VIEWGRANT_SEL_USER;
      break;

    case OT_STATIC_VIEWGRANT_INS_USER:
      return OT_VIEWGRANT_INS_USER;
      break;

    case OT_STATIC_VIEWGRANT_UPD_USER:
      return OT_VIEWGRANT_UPD_USER;
      break;

    case OT_STATIC_VIEWGRANT_DEL_USER:
      return OT_VIEWGRANT_DEL_USER;
      break;

    case OT_STATIC_R_VIEWSYNONYM:
      return OTR_VIEWSYNONYM;
      break;

    // level 2, child of "procedure of database"
    case OT_STATIC_PROCGRANT_EXEC_USER:
      return OT_PROCGRANT_EXEC_USER;
      break;

    // level 2, child of "Sequence of database"
    case OT_STATIC_SEQGRANT_NEXT_USER:
      return OT_SEQUGRANT_NEXT_USER;
      break;

    case OT_STATIC_R_PROC_RULE:
      return OTR_PROC_RULE;
      break;

    // level 2, child of "dbevent of database"
    case OT_STATIC_DBEGRANT_RAISE_USER:
      return OT_DBEGRANT_RAISE_USER;
      break;

    case OT_STATIC_DBEGRANT_REGTR_USER:
      return OT_DBEGRANT_REGTR_USER;
      break;

    // level 2 - child of "location as child of table"
    case OT_TABLELOCATION:
      return OT_STATIC_DUMMY;
      break;

    case OT_STATIC_R_LOCATIONTABLE:
      return OTR_LOCATIONTABLE;
      break;

    // pseudo level 2 - to determine the type of object
    // lying under a viewtable
    // i.e: is the view component a table or a view ?
    case OT_VIEWTABLE:
      if (bNoSolve)
        return OT_VIEWTABLE;
      else 
        return OT_VIEWTABLE_SOLVE;      // TO BE FINISHED : 2 TYPES SOLVE
      break;

    // level 3, child of "index on table of database"
    case OT_STATIC_R_INDEXSYNONYM:
      return OTR_INDEXSYNONYM;
      break;

    // level 3, child of "rule on table of database"
    case OT_STATIC_RULEPROC:
      return OT_RULEPROC;
      break;

    // replicator - all levels mixed
    case OT_STATIC_REPLICATOR:
      return OT_STATIC_DUMMY;

    case OT_STATIC_REPLIC_CONNECTION:
      return OT_REPLIC_CONNECTION;
    case OT_STATIC_REPLIC_CDDS:
      return OT_REPLIC_CDDS;
    case OT_STATIC_REPLIC_MAILUSER:
      return OT_REPLIC_MAILUSER;
    case OT_STATIC_REPLIC_REGTABLE:
      return OT_REPLIC_REGTABLE;

    case OT_REPLIC_CONNECTION:
    case OT_REPLIC_CDDS:
    case OT_REPLIC_MAILUSER:
    case OT_REPLIC_REGTABLE:
      return OT_STATIC_DUMMY;

    case OT_STATIC_REPLIC_CDDS_DETAIL:
      return OT_REPLIC_CDDS_DETAIL;
    case OT_STATIC_R_REPLIC_CDDS_TABLE:
      return OTR_REPLIC_CDDS_TABLE;

    case OT_REPLIC_CDDS_DETAIL:
    case OTR_REPLIC_CDDS_TABLE:
      return OT_STATIC_DUMMY;

    case OT_STATIC_R_REPLIC_TABLE_CDDS:
      return OTR_REPLIC_TABLE_CDDS;

    case OTR_REPLIC_TABLE_CDDS:
      return OT_STATIC_DUMMY;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_STATIC_ALARMEE:
    case OT_STATIC_ALARM_EVENT:
      return OT_STATIC_DUMMY;   // SPECIAL SINCE CREATED IN ONE SHOT

    //
    // ICE
    //
    case OT_STATIC_ICE                      :
      return OT_ICE_MAIN;
    // Under "Security"
    case OT_STATIC_ICE_SECURITY             :
      return OT_STATIC_DUMMY;
    case OT_STATIC_ICE_ROLE                 :
      return OT_ICE_ROLE;
    case OT_STATIC_ICE_DBUSER               :
      return OT_ICE_DBUSER;
    case OT_STATIC_ICE_DBCONNECTION         :
      return OT_ICE_DBCONNECTION;
    case OT_STATIC_ICE_WEBUSER              :
      return OT_ICE_WEBUSER;
    case OT_STATIC_ICE_WEBUSER_ROLE         :
      return OT_ICE_WEBUSER_ROLE;
    case OT_STATIC_ICE_WEBUSER_CONNECTION   :
      return OT_ICE_WEBUSER_CONNECTION;
    case OT_STATIC_ICE_PROFILE              :
      return OT_ICE_PROFILE;
    case OT_STATIC_ICE_PROFILE_ROLE         :
      return OT_ICE_PROFILE_ROLE;
    case OT_STATIC_ICE_PROFILE_CONNECTION   :
      return OT_ICE_PROFILE_CONNECTION;
    // Under "Bussiness unit" (BUNIT)
    case OT_STATIC_ICE_BUNIT                :
      return OT_ICE_BUNIT;
    case OT_STATIC_ICE_BUNIT_SECURITY       :
      return OT_STATIC_DUMMY;
    case OT_STATIC_ICE_BUNIT_SEC_ROLE       :
      return OT_ICE_BUNIT_SEC_ROLE;
    case OT_STATIC_ICE_BUNIT_SEC_USER       :
      return OT_ICE_BUNIT_SEC_USER;
    case OT_STATIC_ICE_BUNIT_FACET          :
      return OT_ICE_BUNIT_FACET;
    case OT_STATIC_ICE_BUNIT_PAGE           :
      return OT_ICE_BUNIT_PAGE;

    case OT_STATIC_ICE_BUNIT_FACET_ROLE     :
      return OT_ICE_BUNIT_FACET_ROLE;
    case OT_STATIC_ICE_BUNIT_FACET_USER     :
      return OT_ICE_BUNIT_FACET_USER;
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE     :
      return OT_ICE_BUNIT_PAGE_ROLE;
    case OT_STATIC_ICE_BUNIT_PAGE_USER     :
      return OT_ICE_BUNIT_PAGE_USER;

    case OT_STATIC_ICE_BUNIT_LOCATION       :
      return OT_ICE_BUNIT_LOCATION;
    // Under "Server"
    case OT_STATIC_ICE_SERVER               :
      return OT_STATIC_DUMMY;
    case OT_STATIC_ICE_SERVER_APPLICATION   :
      return OT_ICE_SERVER_APPLICATION;
    case OT_STATIC_ICE_SERVER_LOCATION      :
      return OT_ICE_SERVER_LOCATION;
    case OT_STATIC_ICE_SERVER_VARIABLE      :
      return OT_ICE_SERVER_VARIABLE;

    //
    // INSTALLATION LEVEL SETTINGS
    //
    case OT_STATIC_INSTALL:
    case OT_STATIC_INSTALL_SECURITY:
    case OT_STATIC_INSTALL_GRANTEES:
    case OT_STATIC_INSTALL_ALARMS:
      return OT_STATIC_DUMMY;


    default:
      return OT_STATIC_DUMMY;   // ERROR! UNTESTED CASE

  }

}

//
//  returns the static type corresponding to a non-static type
//  example : for OT_DATABASE, returns OT_STATIC_DATABASE
//
//  Can be considered as the reverse of GetChildType, restricted to static
//
//  IMPORTANT NOTE : this function needs to be updated for every new type
//
int GetStaticType(int iobjecttype)
{
  int recType;

  // Very special
  if (iobjecttype == OT_STATIC_INSTALL_SECURITY)
    return OT_STATIC_INSTALL_SECURITY;

  // Star management: pre-test for multiple icons according to objects subtypes
  switch (iobjecttype) {
    // databases
    case OT_STAR_DB_DDB:
    case OT_STAR_DB_CDB:

    // tables:
    case OT_STAR_TBL_NATIVE:
    case OT_STAR_TBL_LINK:

    // views:
    case OT_STAR_VIEW_NATIVE:
    case OT_STAR_VIEW_LINK:

    // procédures et index
    case OT_STAR_PROCEDURE:
    case OT_STAR_INDEX:

      return iobjecttype;
  }

  switch (iobjecttype) {

    //
    // Zero levels of parenthood
    //

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

    //
    // One levels of parenthood
    //

    // level 1, child of database
    case OTR_CDB:
      recType = OT_STATIC_R_CDB;
      break;

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
      recType  = OT_STATIC_SEQUENCE;  // parentstrings[0] = base name
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
    case OTR_DBGRANT_SEQCRY_DB:
      recType = OT_STATIC_R_DBGRANT_CRESEQY;  // parent: user/group/role
      break;
    case OTR_DBGRANT_SEQCRN_DB:
      recType = OT_STATIC_R_DBGRANT_CRESEQN;  // parent: user/group/role
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
      recType = OT_STATIC_ROLEGRANT_USER;   // parentstring[0] = role name
      break;

    // level 1, child of location
    case OTR_LOCATIONTABLE:
      recType = OT_STATIC_R_LOCATIONTABLE;  // parentstring[0]: location name
      break;

    //
    // Two levels of parenthood
    //

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

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:
      recType = OT_STATIC_SEQGRANT_NEXT_USER;
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

    // level 2, child of "schemauser on database"
    case OT_SCHEMAUSER_TABLE:
      recType  = OT_STATIC_SCHEMAUSER_TABLE; // parentstrings[0] = base name
      break;
    case OT_SCHEMAUSER_VIEW:
      recType  = OT_STATIC_SCHEMAUSER_VIEW;  // parentstrings[0] = base name
      break;
    case OT_SCHEMAUSER_PROCEDURE:
      recType  = OT_STATIC_SCHEMAUSER_PROCEDURE;  // parentstrings[0] = base name
      break;

    //
    // Three levels of parenthood
    //

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

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_ALARMEE:
      recType = OT_STATIC_ALARMEE;
      break;
    case OT_S_ALARM_EVENT:
      recType = OT_STATIC_ALARM_EVENT;
      break;

    case OT_SYNONYMOBJECT:
      recType = OT_STATIC_SYNONYMED;
      break;

    // replicator not installed
    case OT_REPLICATOR:
      recType = OT_STATIC_REPLICATOR;
      break;

    //
    // ICE
    //
    case OT_ICE_SUBSTATICS:
    case OT_ICE_MAIN:
      recType = OT_STATIC_ICE;
      break;
    // Under "Security"
    case OT_ICE_ROLE                 :
      recType = OT_STATIC_ICE_ROLE;
      break;
    case OT_ICE_DBUSER               :
      recType = OT_STATIC_ICE_DBUSER;
      break;
    case OT_ICE_DBCONNECTION         :
      recType = OT_STATIC_ICE_DBCONNECTION;
      break;
    case OT_ICE_WEBUSER              :
      recType = OT_STATIC_ICE_WEBUSER;
      break;
    case OT_ICE_WEBUSER_ROLE         :
      recType = OT_STATIC_ICE_WEBUSER_ROLE;
      break;
    case OT_ICE_WEBUSER_CONNECTION   :
      recType = OT_STATIC_ICE_WEBUSER_CONNECTION;
      break;
    case OT_ICE_PROFILE              :
      recType = OT_STATIC_ICE_PROFILE;
      break;
    case OT_ICE_PROFILE_ROLE         :
      recType = OT_STATIC_ICE_PROFILE_ROLE;
      break;
    case OT_ICE_PROFILE_CONNECTION   :
      recType = OT_STATIC_ICE_PROFILE_CONNECTION;
      break;
    // Under "Bussiness unit" (BUNIT)
    case OT_ICE_BUNIT                :
      recType = OT_STATIC_ICE_BUNIT;
      break;
    case OT_ICE_BUNIT_SEC_ROLE       :
      recType = OT_STATIC_ICE_BUNIT_SEC_ROLE;
      break;
    case OT_ICE_BUNIT_SEC_USER       :
      recType = OT_STATIC_ICE_BUNIT_SEC_USER;
      break;
    case OT_ICE_BUNIT_FACET          :
      recType = OT_STATIC_ICE_BUNIT_FACET;
      break;
    case OT_ICE_BUNIT_PAGE           :
      recType = OT_STATIC_ICE_BUNIT_PAGE;
      break;

    case OT_ICE_BUNIT_FACET_ROLE:
      recType = OT_STATIC_ICE_BUNIT_FACET_ROLE     ;
      break;
    case OT_ICE_BUNIT_FACET_USER:
      recType = OT_STATIC_ICE_BUNIT_FACET_USER     ;
      break;
    case OT_ICE_BUNIT_PAGE_ROLE:
      recType = OT_STATIC_ICE_BUNIT_PAGE_ROLE     ;
      break;
    case OT_ICE_BUNIT_PAGE_USER:
      recType = OT_STATIC_ICE_BUNIT_PAGE_USER     ;
      break;

    case OT_ICE_BUNIT_LOCATION       :
      recType = OT_STATIC_ICE_BUNIT_LOCATION;
      break;
    // Under "Server"
    case OT_ICE_SERVER_APPLICATION   :
      recType = OT_STATIC_ICE_SERVER_APPLICATION;
      break;
    case OT_ICE_SERVER_LOCATION      :
      recType = OT_STATIC_ICE_SERVER_LOCATION;
      break;
    case OT_ICE_SERVER_VARIABLE      :
      recType = OT_STATIC_ICE_SERVER_VARIABLE;
      break;

    default:
      recType = 0;
  }
  return recType;
}

//
//BOOL IsDomDataNeeded (int      hnodestruct,    // handle on node struct
//                      int      iobjecttype,    // object type
//                      int      level,          // parenthood level
//                      LPUCHAR *parentstrings)  // parenthood names
//
// This function says whether, for the given object with the given
// parenthood, at least one dom opened on the given node has already
// expanded the branch
// This function will be used to optimize the save/load environment
//
// Returns TRUE if branch expanded
//
// if the parenthood strings are not sufficient, a "myerror" will
// be called with ERR_ISDOMDATANEEDED, and the function will return TRUE
//
BOOL IsDomDataNeeded (int hnodestruct, int iobjecttype, int level, LPUCHAR *parentstrings)
{
  // TO BE FINISHED!

  // dummy instructions to skip compiler warnings at level four
  hnodestruct;
  iobjecttype;
  level;
  parentstrings;

  return TRUE;
}


//
//  gives the level and the string id for no item,
//  for a given object type
//  Can fill only level if pids_none is NULL, and vice-versa
//
//  returns RES_SUCCESS if successful, RES_ERR if guess what.
//
static int NEAR MakeLevelAndIdsNone(int iobjecttype, int *plevel, UINT *pids_none)
{
  int   level;
  UINT  ids_none;

  switch (iobjecttype) {
    // level 0
    case OT_DATABASE:
      level = 0;
      ids_none = IDS_TREE_DATABASE_NONE;
      break;
    case OT_PROFILE:
      level = 0;
      ids_none = IDS_TREE_PROFILE_NONE;
      break;
    case OT_USER:
      level = 0;
      ids_none = IDS_TREE_USER_NONE;
      break;
    case OT_GROUP:
      level = 0;
      ids_none = IDS_TREE_GROUP_NONE;
      break;
    case OT_ROLE:
      level = 0;
      ids_none = IDS_TREE_ROLE_NONE;
      break;
    case OT_LOCATION:
      level = 0;
      ids_none = IDS_TREE_LOCATION_NONE;
      break;

    // level 1, child of database
    case OTR_CDB:
      level = 1;
      ids_none = IDS_TREE_R_CDB_NONE;
      break;
    case OT_TABLE:
      level = 1;
      ids_none = IDS_TREE_TABLE_NONE;
      break;
    case OT_VIEW:
      level = 1;
      ids_none = IDS_TREE_VIEW_NONE;
      break;
    case OT_PROCEDURE:
      level = 1;
      ids_none = IDS_TREE_PROCEDURE_NONE;
      break;
    case OT_SEQUENCE:
      level = 1;
      ids_none = IDS_TREE_SEQUENCE_NONE;
      break;
    case OT_SCHEMAUSER:
      level = 1;
      ids_none = IDS_TREE_SCHEMA_NONE;
      break;
    case OT_SYNONYM:
      level = 1;
      ids_none = IDS_TREE_SYNONYM_NONE;
      break;
    case OT_DBGRANT_ACCESY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_ACCESY_NONE;
      break;
    case OT_DBGRANT_ACCESN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_ACCESN_NONE;
      break;
    case OT_DBGRANT_CREPRY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CREPRY_NONE;
      break;
    case OT_DBGRANT_CREPRN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CREPRN_NONE;
      break;
    case OT_DBGRANT_CRETBY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CRETBY_NONE;
      break;
    case OT_DBGRANT_CRETBN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CRETBN_NONE;
      break;
    case OT_DBGRANT_DBADMY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_DBADMY_NONE;
      break;
    case OT_DBGRANT_DBADMN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_DBADMN_NONE;
      break;
    case OT_DBGRANT_LKMODY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_LKMODY_NONE;
      break;
    case OT_DBGRANT_LKMODN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_LKMODN_NONE;
      break;
    case OT_DBGRANT_QRYIOY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYIOY_NONE;
      break;
    case OT_DBGRANT_QRYION_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYION_NONE;
      break;
    case OT_DBGRANT_QRYRWY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYRWY_NONE;
      break;
    case OT_DBGRANT_QRYRWN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYRWN_NONE;
      break;
    case OT_DBGRANT_UPDSCY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_UPDSCY_NONE;
      break;
    case OT_DBGRANT_UPDSCN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_UPDSCN_NONE;
      break;

    case OT_DBGRANT_SELSCY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_SELSCY_NONE;
      break;
    case OT_DBGRANT_SELSCN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_SELSCN_NONE;
      break;
    case OT_DBGRANT_CNCTLY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CNCTLY_NONE;
      break;
    case OT_DBGRANT_CNCTLN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CNCTLN_NONE;
      break;
    case OT_DBGRANT_IDLTLY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_IDLTLY_NONE;
      break;
    case OT_DBGRANT_IDLTLN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_IDLTLN_NONE;
      break;
    case OT_DBGRANT_SESPRY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_SESPRY_NONE;
      break;
    case OT_DBGRANT_SESPRN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_SESPRN_NONE;
      break;
    case OT_DBGRANT_TBLSTY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_TBLSTY_NONE;
      break;
    case OT_DBGRANT_TBLSTN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_TBLSTN_NONE;
      break;
    case OT_DBGRANT_QRYCPY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYCPY_NONE;
      break;
    case OT_DBGRANT_QRYCPN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYCPN_NONE;
      break;
    case OT_DBGRANT_QRYPGY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYPGY_NONE;
      break;
    case OT_DBGRANT_QRYPGN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYPGN_NONE;
      break;
    case OT_DBGRANT_QRYCOY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYCOY_NONE;
      break;
    case OT_DBGRANT_QRYCON_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_QRYCON_NONE;
      break;
    case OT_DBGRANT_SEQCRY_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CRSEQY_NONE;
      break;
    case OT_DBGRANT_SEQCRN_USER:
      level = 1;
      ids_none = IDS_TREE_DBGRANTEES_CRSEQN_NONE;
      break;

    case OT_DBEVENT:
      level = 1;
      ids_none = IDS_TREE_DBEVENT_NONE;
      break;

    case OT_S_ALARM_CO_SUCCESS_USER:
      level = 1;
      ids_none = IDS_TREE_DBALARM_CONN_SUCCESS_NONE;
      break;
    case OT_S_ALARM_CO_FAILURE_USER:
      level = 1;
      ids_none = IDS_TREE_DBALARM_CONN_FAILURE_NONE;
      break;
    case OT_S_ALARM_DI_SUCCESS_USER:
      level = 1;
      ids_none = IDS_TREE_DBALARM_DISCONN_SUCCESS_NONE;
      break;
    case OT_S_ALARM_DI_FAILURE_USER:
      level = 1;
      ids_none = IDS_TREE_DBALARM_DISCONN_FAILURE_NONE;
      break;

    // level 1, child of database/replicator
    case OT_REPLIC_CONNECTION:
      level = 1;
      ids_none = IDS_TREE_REPLIC_CONNECTION_NONE;
      break;
    case OT_REPLIC_CDDS:
      level = 1;
      ids_none = IDS_TREE_REPLIC_CDDS_NONE;
      break;
    case OT_REPLIC_MAILUSER:
      level = 1;
      ids_none = IDS_TREE_REPLIC_MAILUSER_NONE;
      break;
    case OT_REPLIC_REGTABLE:
      level = 1;
      ids_none = IDS_TREE_REPLIC_REGTABLE_NONE;
      break;

    // level 1, child of user
    case OTR_USERSCHEMA:
      level = 1;
      ids_none = IDS_TREE_R_USERSCHEMA_NONE;
      break;
    case OTR_USERGROUP:
      level = 1;
      ids_none = IDS_TREE_R_USERGROUP_NONE;
      break;

    case OTR_ALARM_SELSUCCESS_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_SEL_SUCC_NONE;
      break;
    case OTR_ALARM_SELFAILURE_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_SEL_FAIL_NONE;
      break;
    case OTR_ALARM_DELSUCCESS_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_DEL_SUCC_NONE;
      break;
    case OTR_ALARM_DELFAILURE_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_DEL_FAIL_NONE;
      break;
    case OTR_ALARM_INSSUCCESS_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_INS_SUCC_NONE;
      break;
    case OTR_ALARM_INSFAILURE_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_INS_FAIL_NONE;
      break;
    case OTR_ALARM_UPDSUCCESS_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_UPD_SUCC_NONE;
      break;
    case OTR_ALARM_UPDFAILURE_TABLE:
      level = 1;
      ids_none = IDS_TREE_R_SEC_UPD_FAIL_NONE;
      break;

    case OTR_GRANTEE_RAISE_DBEVENT:
      level = 1;
      ids_none = IDS_TREE_R_DBEGRANT_RAISE_NONE;
      break;
    case OTR_GRANTEE_REGTR_DBEVENT:
      level = 1;
      ids_none = IDS_TREE_R_DBEGRANT_REGISTER_NONE;
      break;
    case OTR_GRANTEE_EXEC_PROC:
      level = 1;
      ids_none = IDS_TREE_R_PROCGRANT_EXEC_NONE;
      break;
    case OTR_GRANTEE_NEXT_SEQU:
      level = 1;
      ids_none = IDS_TREE_R_SEQGRANT_NEXT_NONE;
      break;
    case OTR_DBGRANT_ACCESY_DB:
      level = 1;
      ids_none = IDS_TREE_R_DBGRANT_ACCESY_NONE;
      break;
    case OTR_DBGRANT_ACCESN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_ACCESN_NONE;
      break;
    case OTR_DBGRANT_CREPRY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CREPRY_NONE;
      break;
    case OTR_DBGRANT_CREPRN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CREPRN_NONE;
      break;
    case OTR_DBGRANT_SEQCRY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CRESEQY_NONE;
      break;
    case OTR_DBGRANT_SEQCRN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CRESEQN_NONE;
      break;
    case OTR_DBGRANT_CRETBY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CRETBY_NONE;
      break;
    case OTR_DBGRANT_CRETBN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CRETBN_NONE;
      break;
    case OTR_DBGRANT_DBADMY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_DBADMY_NONE;
      break;
    case OTR_DBGRANT_DBADMN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_DBADMN_NONE;
      break;
    case OTR_DBGRANT_LKMODY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_LKMODY_NONE;
      break;
    case OTR_DBGRANT_LKMODN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_LKMODN_NONE;
      break;
    case OTR_DBGRANT_QRYIOY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYIOY_NONE;
      break;
    case OTR_DBGRANT_QRYION_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYION_NONE;
      break;
    case OTR_DBGRANT_QRYRWY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYRWY_NONE;
      break;
    case OTR_DBGRANT_QRYRWN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYRWN_NONE;
      break;
    case OTR_DBGRANT_UPDSCY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_UPDSCY_NONE;
      break;
    case OTR_DBGRANT_UPDSCN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_UPDSCN_NONE;
      break;

    case OTR_DBGRANT_SELSCY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_SELSCY_NONE;
      break;
    case OTR_DBGRANT_SELSCN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_SELSCN_NONE;
      break;
    case OTR_DBGRANT_CNCTLY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CNCTLY_NONE;
      break;
    case OTR_DBGRANT_CNCTLN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_CNCTLN_NONE;
      break;
    case OTR_DBGRANT_IDLTLY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_IDLTLY_NONE;
      break;
    case OTR_DBGRANT_IDLTLN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_IDLTLN_NONE;
      break;
    case OTR_DBGRANT_SESPRY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_SESPRY_NONE;
      break;
    case OTR_DBGRANT_SESPRN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_SESPRN_NONE;
      break;
    case OTR_DBGRANT_TBLSTY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_TBLSTY_NONE;
      break;
    case OTR_DBGRANT_TBLSTN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_TBLSTN_NONE;
      break;
    case OTR_DBGRANT_QRYCPY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYCPY_NONE;
      break;
    case OTR_DBGRANT_QRYCPN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYCPN_NONE;
      break;
    case OTR_DBGRANT_QRYPGY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYPGY_NONE;
      break;
    case OTR_DBGRANT_QRYPGN_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYPGN_NONE;
      break;
    case OTR_DBGRANT_QRYCOY_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYCOY_NONE;
      break;
    case OTR_DBGRANT_QRYCON_DB:
      level = 1;
      ids_none =  IDS_TREE_R_DBGRANT_QRYCON_NONE;
      break;

    case OTR_GRANTEE_SEL_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_SEL_NONE;
      break;
    case OTR_GRANTEE_INS_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_INS_NONE;
      break;
    case OTR_GRANTEE_UPD_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_UPD_NONE;
      break;
    case OTR_GRANTEE_DEL_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_DEL_NONE;
      break;
    case OTR_GRANTEE_REF_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_REF_NONE;
      break;
    case OTR_GRANTEE_CPI_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_CPI_NONE;
      break;
    case OTR_GRANTEE_CPF_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_CPF_NONE;
      break;
    case OTR_GRANTEE_ALL_TABLE:
      level = 1;
      ids_none =  IDS_TREE_R_TABLEGRANT_ALL_NONE;
      break;
    case OTR_GRANTEE_SEL_VIEW:
      level = 1;
      ids_none =  IDS_TREE_R_VIEWGRANT_SEL_NONE;
      break;
    case OTR_GRANTEE_INS_VIEW:
      level = 1;
      ids_none =  IDS_TREE_R_VIEWGRANT_INS_NONE;
      break;
    case OTR_GRANTEE_UPD_VIEW:
      level = 1;
      ids_none =  IDS_TREE_R_VIEWGRANT_UPD_NONE;
      break;
    case OTR_GRANTEE_DEL_VIEW:
      level = 1;
      ids_none =  IDS_TREE_R_VIEWGRANT_DEL_NONE;
      break;
    case OTR_GRANTEE_ROLE:
      level = 1;
      ids_none =  IDS_TREE_R_ROLEGRANT_NONE;
      break;

    // level 1, child of group
    case OT_GROUPUSER:
      level = 1;
      ids_none = IDS_TREE_GROUPUSER_NONE;
      break;

    // level 1, child of role
    case OT_ROLEGRANT_USER:
      level = 1;
      ids_none = IDS_TREE_ROLEGRANT_USER_NONE;
      break;

    // level 1, child of location
    case OTR_LOCATIONTABLE:
      level = 1;
      ids_none = IDS_TREE_R_LOCATIONTABLE_NONE;
      break;

    // level 2, child of "table of database"
    case OT_INTEGRITY:
      level = 2;
      ids_none = IDS_TREE_INTEGRITY_NONE;
      break;
    case OT_RULE:
      level = 2;
      ids_none = IDS_TREE_RULE_NONE;
      break;
    case OT_INDEX:
      level = 2;
      ids_none = IDS_TREE_INDEX_NONE;
      break;
    case OT_TABLELOCATION:
      level = 2;
      ids_none = IDS_TREE_TABLELOCATION_NONE;
      break;
    case OT_S_ALARM_SELSUCCESS_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_SEL_SUCC_NONE;
      break;
    case OT_S_ALARM_SELFAILURE_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_SEL_FAIL_NONE;
      break;
    case OT_S_ALARM_DELSUCCESS_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_DEL_SUCC_NONE;
      break;
    case OT_S_ALARM_DELFAILURE_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_DEL_FAIL_NONE;
      break;
    case OT_S_ALARM_INSSUCCESS_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_INS_SUCC_NONE;
      break;
    case OT_S_ALARM_INSFAILURE_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_INS_FAIL_NONE;
      break;
    case OT_S_ALARM_UPDSUCCESS_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_UPD_SUCC_NONE;
      break;
    case OT_S_ALARM_UPDFAILURE_USER:
      level = 2;
      ids_none = IDS_TREE_SEC_UPD_FAIL_NONE;
      break;
    case OT_TABLEGRANT_SEL_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_SEL_USER_NONE;
      break;
    case OT_TABLEGRANT_INS_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_INS_USER_NONE;
      break;
    case OT_TABLEGRANT_UPD_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_UPD_USER_NONE;
      break;
    case OT_TABLEGRANT_DEL_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_DEL_USER_NONE;
      break;
    case OT_TABLEGRANT_REF_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_REF_USER_NONE;
      break;
    case OT_TABLEGRANT_CPI_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_CPI_USER_NONE;
      break;
    case OT_TABLEGRANT_CPF_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_CPF_USER_NONE;
      break;
    case OT_TABLEGRANT_ALL_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_ALL_USER_NONE;
      break;
    // desktop
    case OT_TABLEGRANT_INDEX_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_INDEX_USER_NONE;
      break;
    // desktop
    case OT_TABLEGRANT_ALTER_USER:
      level = 2;
      ids_none = IDS_TREE_TABLEGRANT_ALTER_USER_NONE;
      break;
    case OTR_TABLESYNONYM:
      level = 2;
      ids_none = IDS_TREE_R_TABLESYNONYM_NONE;
      break;
    case OTR_TABLEVIEW:
      level = 2;
      ids_none = IDS_TREE_R_TABLEVIEW_NONE;
      break;
    case OTR_REPLIC_TABLE_CDDS:
      level = 2;
      ids_none = IDS_TREE_R_REPLIC_TABLE_CDDS_NONE;
      break;

    // level 2, child of "view of database"
    case OT_VIEWTABLE:
      level = 2;
      ids_none = IDS_TREE_VIEWTABLE_NONE;
      break;
    case OT_VIEWGRANT_SEL_USER:
      level = 2;
      ids_none = IDS_TREE_VIEWGRANT_SEL_USER_NONE;
      break;
    case OT_VIEWGRANT_INS_USER:
      level = 2;
      ids_none = IDS_TREE_VIEWGRANT_INS_USER_NONE;
      break;
    case OT_VIEWGRANT_UPD_USER:
      level = 2;
      ids_none = IDS_TREE_VIEWGRANT_UPD_USER_NONE;
      break;
    case OT_VIEWGRANT_DEL_USER:
      level = 2;
      ids_none = IDS_TREE_VIEWGRANT_DEL_USER_NONE;
      break;
    case OTR_VIEWSYNONYM:
      level = 2;
      ids_none = IDS_TREE_R_VIEWSYNONYM_NONE;
      break;

    // level 2, child of "procedure of database"
    case OT_PROCGRANT_EXEC_USER:
      level = 2;
      ids_none = IDS_TREE_PROCGRANT_EXEC_USER_NONE;
      break;
    case OTR_PROC_RULE:
      level = 2;
      ids_none = IDS_TREE_R_PROC_RULE_NONE;
      break;

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:
      level = 2;
      ids_none = IDS_TREE_SEQUENCE_GRANT_NONE;
      break;

    // level 2, child of "dbevent of database"
    case OT_DBEGRANT_RAISE_USER:
      level = 2;
      ids_none = IDS_TREE_DBEGRANT_RAISE_USER_NONE;
      break;
    case OT_DBEGRANT_REGTR_USER:
      level = 2;
      ids_none = IDS_TREE_DBEGRANT_REGTR_USER_NONE;
      break;

    // level 2, child of "cdds of database"
    case OT_REPLIC_CDDS_DETAIL:
      level = 2;
      ids_none = IDS_TREE_REPLIC_CDDS_DETAIL_NONE;
      break;
    case OTR_REPLIC_CDDS_TABLE:
      level = 2;
      ids_none = IDS_TREE_R_REPLIC_CDDS_TABLE_NONE;
      break;

    // level 2, child of "schemauser on database"
    case OT_SCHEMAUSER_TABLE:
      level = 2;
      ids_none = IDS_TREE_SCHEMAUSER_TABLE_NONE;
      break;
    case OT_SCHEMAUSER_VIEW:
      level = 2;
      ids_none = IDS_TREE_SCHEMAUSER_VIEW_NONE;
      break;
    case OT_SCHEMAUSER_PROCEDURE:
      level = 2;
      ids_none = IDS_TREE_SCHEMAUSER_PROCEDURE_NONE;
      break;

    // level 3, child of "index on view of database"
    case OTR_INDEXSYNONYM:
      level = 3;
      ids_none = IDS_TREE_R_INDEXSYNONYM_NONE;
      break;

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:
      level = 3;
      ids_none = IDS_TREE_RULEPROC_NONE;
      break;

    //
    // ICE
    //
    case OT_ICE_SUBSTATICS           :
      level = 0;
      ids_none = IDS_TREE_ICE_SERVER_NONE;
      break;
    // Under "Security"
    case OT_ICE_ROLE                 :
      level = 0;
      ids_none = IDS_TREE_ICE_ROLE_NONE;
      break;
    case OT_ICE_DBUSER               :
      level = 0;
      ids_none = IDS_TREE_ICE_DBUSER_NONE;
      break;
    case OT_ICE_DBCONNECTION         :
      level = 0;
      ids_none = IDS_TREE_ICE_DBCONNECTION_NONE;
      break;
    case OT_ICE_WEBUSER              :
      level = 0;
      ids_none = IDS_TREE_ICE_WEBUSER_NONE;
      break;
    case OT_ICE_WEBUSER_ROLE         :
      level = 1;
      ids_none = IDS_TREE_ICE_WEBUSER_ROLE_NONE;
      break;
    case OT_ICE_WEBUSER_CONNECTION   :
      level = 1;
      ids_none = IDS_TREE_ICE_WEBUSER_CONNECTION_NONE;
      break;
    case OT_ICE_PROFILE              :
      level = 0;
      ids_none = IDS_TREE_ICE_PROFILE_NONE;
      break;
    case OT_ICE_PROFILE_ROLE         :
      level = 1;
      ids_none = IDS_TREE_ICE_PROFILE_ROLE_NONE;
      break;
    case OT_ICE_PROFILE_CONNECTION   :
      level = 1;
      ids_none = IDS_TREE_ICE_PROFILE_CONNECTION_NONE;
      break;
    // Under "Bussiness unit" (BUNIT)
    case OT_ICE_BUNIT                :
      level = 0;
      ids_none = IDS_TREE_ICE_BUNIT_NONE;
      break;
    case OT_ICE_BUNIT_SEC_ROLE       :
      level = 1;
      ids_none = IDS_TREE_ICE_BUNIT_SEC_ROLE_NONE;
      break;
    case OT_ICE_BUNIT_SEC_USER       :
      level = 1;
      ids_none = IDS_TREE_ICE_BUNIT_SEC_USER_NONE;
      break;
    case OT_ICE_BUNIT_FACET          :
      level = 1;
      ids_none = IDS_TREE_ICE_BUNIT_FACET_NONE;
      break;
    case OT_ICE_BUNIT_PAGE           :
      level = 1;
      ids_none = IDS_TREE_ICE_BUNIT_PAGE_NONE;
      break;

    case OT_ICE_BUNIT_FACET_ROLE     :
      level = 2;
      ids_none = IDS_TREE_ICE_BUNIT_FACET_ROLE_NONE;
      break;
    case OT_ICE_BUNIT_FACET_USER     :
      level = 2;
      ids_none = IDS_TREE_ICE_BUNIT_FACET_USER_NONE;
      break;
    case OT_ICE_BUNIT_PAGE_ROLE     :
      level = 2;
      ids_none = IDS_TREE_ICE_BUNIT_PAGE_ROLE_NONE;
      break;
    case OT_ICE_BUNIT_PAGE_USER     :
      level = 2;
      ids_none = IDS_TREE_ICE_BUNIT_PAGE_USER_NONE;
      break;

    case OT_ICE_BUNIT_LOCATION       :
      level = 1;
      ids_none = IDS_TREE_ICE_BUNIT_LOCATION_NONE;
      break;
    // Under "Server"
    case OT_ICE_SERVER_APPLICATION   :
      level = 0;
      ids_none = IDS_TREE_ICE_SERVER_APPLICATION_NONE;
      break;
    case OT_ICE_SERVER_LOCATION      :
      level = 0;
      ids_none = IDS_TREE_ICE_SERVER_LOCATION_NONE;
      break;
    case OT_ICE_SERVER_VARIABLE      :
      level = 0;
      ids_none = IDS_TREE_ICE_SERVER_VARIABLE_NONE;
      break;

    default:
      return RES_ERR;
  }
  if (plevel)
    *plevel     = level;
  if (pids_none)
    *pids_none  = ids_none;
  return RES_SUCCESS;
}


/********************************************************************
//
//  NEW FUNCTIONS FOR SPEED ENHANCEMENT IN THE TREE:
//
//  - UPDATE WITH OT_VIRTNODE
//
//    Author : Emmanuel Blattes
//
********************************************************************/

static BOOL NEAR RecordFits(LPDOMDATA lpDomData, int iobjecttype, int iAction, LPUCHAR *parentstrings, DWORD id, LPTREERECORD lpRecord, int *presultType, BOOL *pbRootItem);

//
// DWORD GetAnyAnchorId(LPDOMDATA lpDomData,
//                      int       iobjecttype,
//                      int       level,
//                      LPUCHAR  *parentstrings,
//                      DWORD     previousId,
//                      LPUCHAR   bufPar1,
//                      LPUCHAR   bufPar2,
//                      LPUCHAR   bufPar3,
//                      int       iAction,
//                      DWORD     idItem,
//                      int      *piresultobjecttype,
//                      int      *plevel,
//                      UINT     *pids_none,
//                      BOOL      bTryCurrent,
//                      BOOL     *pbRootItem);
//
//  Find the next id of a line in the tree starting from previousId
//  or the first if previousId is 0L,
//  of any static type
//
//  returns 0L if no more line found
//
//  receives a bool saying whether we should try the received idItem
//  (set to true if the previous anchor was a root item and was deleted)
//
//  If line found, returns :
//        - objecttype found (not necessarily same as received iobjecttype
//        - necessary level
//        - "none" string id
//        - parentage data in bufPar1 and bufPar2
//        - root item yes/no
//
//
DWORD GetAnyAnchorId(LPDOMDATA lpDomData, int iobjecttype, int level,
                     LPUCHAR *parentstrings, DWORD previousId,
                     LPUCHAR bufPar1, LPUCHAR bufPar2, LPUCHAR bufPar3,
                     int iAction, DWORD idItem, int *piresultobjecttype,
                     int *plevel, UINT *pids_none,
                     BOOL bTryCurrent, BOOL *pbRootItem)
{
  DWORD         id;
  LPTREERECORD  lpRecord;
  int           resultType;
  BOOL          bRootItem;

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

  // advance in the tree starting from previous id
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
    if (RecordFits(lpDomData, iobjecttype, iAction, parentstrings, id,
                   lpRecord, &resultType, &bRootItem)) {
      MakeLevelAndIdsNone(resultType, plevel, pids_none);
      *piresultobjecttype = resultType;
      lstrcpy(bufPar1, lpRecord->extra);
      lstrcpy(bufPar2, lpRecord->extra2);
      lstrcpy(bufPar3, lpRecord->extra3);
      *pbRootItem = bRootItem;
      return id;
    }
    id = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, (LPARAM)id);
  }
  return 0L;    // NOT FOUND!
}

//
//  for a requested objecttype and parentstrings,
//  says whether the item record is receivable as an anchor block
//
static BOOL NEAR RecordFits(LPDOMDATA lpDomData, int iobjecttype, int iAction, LPUCHAR *parentstrings, DWORD id, LPTREERECORD lpRecord, int *presultType, BOOL *pbRootItem)
{
  int resultType;

  // default value for root item flag
  *pbRootItem = FALSE;

  // keep either static items with potential valid sub-items,
  // or non-static items at root level
  if (IsItemObjStatic(lpDomData, id)) {
    resultType = GetChildType(lpRecord->recType, FALSE);
    if (resultType == OT_STATIC_DUMMY)
      return FALSE;   // item only has sub-static items
  }
  else {
    if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                    0, (LPARAM)id) == 0) {
      *pbRootItem = TRUE;
      resultType = lpRecord->recType;
    }
    else
      return FALSE;   // non-static item is not at level 0
  }

  // VIRTNODE
  if (iobjecttype == OT_VIRTNODE) {

    // filter according to combination between iAction and resultType
    switch (iAction) {
      case ACT_CHANGE_BASE_FILTER:
        if (resultType != OT_DATABASE)
          return FALSE;
        break;

      case ACT_CHANGE_OTHER_FILTER:
        if (!NonDBObjectHasOwner(resultType))
          return FALSE;
        break;

      case ACT_CHANGE_SYSTEMOBJECTS:
        if (!HasSystemObjects(GetBasicType(resultType)))
          return FALSE;
        break;
    }

    // update return values and return
    *presultType = resultType;
    if (resultType == OT_ICE_MAIN) // This branch was updated with the UpdateIceDisplayBranch() function.
        return FALSE;

    return TRUE;
  }

  // OTLL_DBGRANTEE
  if (iobjecttype == OTLL_DBGRANTEE) {
    switch (resultType) {
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
      case OT_DBGRANT_SEQCRN_USER:
      case OT_DBGRANT_SEQCRY_USER:

      case OTR_DBGRANT_ACCESY_DB:
      case OTR_DBGRANT_ACCESN_DB:
      case OTR_DBGRANT_CREPRY_DB:
      case OTR_DBGRANT_CREPRN_DB:
      case OTR_DBGRANT_SEQCRY_DB:
      case OTR_DBGRANT_SEQCRN_DB:
      case OTR_DBGRANT_CRETBY_DB:
      case OTR_DBGRANT_CRETBN_DB:
      case OTR_DBGRANT_DBADMY_DB:
      case OTR_DBGRANT_DBADMN_DB:
      case OTR_DBGRANT_LKMODY_DB:
      case OTR_DBGRANT_LKMODN_DB:
      case OTR_DBGRANT_QRYIOY_DB:
      case OTR_DBGRANT_QRYION_DB:
      case OTR_DBGRANT_QRYRWY_DB:
      case OTR_DBGRANT_QRYRWN_DB:
      case OTR_DBGRANT_UPDSCY_DB:
      case OTR_DBGRANT_UPDSCN_DB:

      case OTR_DBGRANT_SELSCY_DB:
      case OTR_DBGRANT_SELSCN_DB:
      case OTR_DBGRANT_CNCTLY_DB:
      case OTR_DBGRANT_CNCTLN_DB:
      case OTR_DBGRANT_IDLTLY_DB:
      case OTR_DBGRANT_IDLTLN_DB:
      case OTR_DBGRANT_SESPRY_DB:
      case OTR_DBGRANT_SESPRN_DB:
      case OTR_DBGRANT_TBLSTY_DB:
      case OTR_DBGRANT_TBLSTN_DB:

      case OTR_DBGRANT_QRYCPY_DB:
      case OTR_DBGRANT_QRYCPN_DB:
      case OTR_DBGRANT_QRYPGY_DB:
      case OTR_DBGRANT_QRYPGN_DB:
      case OTR_DBGRANT_QRYCOY_DB:
      case OTR_DBGRANT_QRYCON_DB:
        *presultType = resultType;
        return TRUE;
      default:
        return FALSE;
    }
  }

  // OTLL_GRANTEE
  if (iobjecttype == OTLL_GRANTEE) {
    switch (resultType) {
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

      case OT_ROLEGRANT_USER:
      case OTR_GRANTEE_SEL_TABLE:
      case OTR_GRANTEE_INS_TABLE:
      case OTR_GRANTEE_UPD_TABLE:
      case OTR_GRANTEE_DEL_TABLE:
      case OTR_GRANTEE_REF_TABLE:
      case OTR_GRANTEE_CPI_TABLE:
      case OTR_GRANTEE_CPF_TABLE:
      case OTR_GRANTEE_ALL_TABLE:
      case OTR_GRANTEE_SEL_VIEW:
      case OTR_GRANTEE_INS_VIEW:
      case OTR_GRANTEE_UPD_VIEW:
      case OTR_GRANTEE_DEL_VIEW:
      case OTR_GRANTEE_RAISE_DBEVENT:
      case OTR_GRANTEE_REGTR_DBEVENT:
      case OTR_GRANTEE_EXEC_PROC:
      case OTR_GRANTEE_NEXT_SEQU:
      case OTR_GRANTEE_ROLE:
        *presultType = resultType;
        return TRUE;
      default:
        return FALSE;
    }
  }

  // OTLL_SECURITYALARM
  if (iobjecttype == OTLL_SECURITYALARM) {
    switch (resultType) {
      case OT_S_ALARM_SELSUCCESS_USER:
      case OT_S_ALARM_SELFAILURE_USER:
      case OT_S_ALARM_DELSUCCESS_USER:
      case OT_S_ALARM_DELFAILURE_USER:
      case OT_S_ALARM_INSSUCCESS_USER:
      case OT_S_ALARM_INSFAILURE_USER:
      case OT_S_ALARM_UPDSUCCESS_USER:
      case OT_S_ALARM_UPDFAILURE_USER:

      case OTR_ALARM_SELSUCCESS_TABLE:
      case OTR_ALARM_SELFAILURE_TABLE:
      case OTR_ALARM_DELSUCCESS_TABLE:
      case OTR_ALARM_DELFAILURE_TABLE:
      case OTR_ALARM_INSSUCCESS_TABLE:
      case OTR_ALARM_INSFAILURE_TABLE:
      case OTR_ALARM_UPDSUCCESS_TABLE:
      case OTR_ALARM_UPDFAILURE_TABLE:

        *presultType = resultType;
        return TRUE;
      default:
        return FALSE;
    }
  }

  // OTLL_DBSECALARM
  if (iobjecttype == OTLL_DBSECALARM) {
    switch (resultType) {
      case OT_S_ALARM_CO_SUCCESS_USER:
      case OT_S_ALARM_CO_FAILURE_USER:
      case OT_S_ALARM_DI_SUCCESS_USER:
      case OT_S_ALARM_DI_FAILURE_USER:
        *presultType = resultType;
        return TRUE;
      default:
        return FALSE;
    }
  }

  // OTLL_OIDTDBGRANTEE
  if (iobjecttype == OTLL_OIDTDBGRANTEE) {
    switch (resultType) {
      case OT_DBGRANT_ACCESY_USER:  // mapped on Connect grant
      case OT_DBGRANT_DBADMY_USER:  // mapped on Dba grant
      case OT_DBGRANT_CRETBY_USER:  // mapped on Resource grant
        *presultType = resultType;
        return TRUE;
      default:
        return FALSE;
    }
  }

  // other cases not managed yet!
  return FALSE;
}
static BOOL FillReplicLocalDBInformation(int hnode, LPTREERECORD  lpRecord, HWND hwndParent ,int ReplicVersion)
{
  int      iret;
  UCHAR    objName[MAXOBJECTNAME];    // result object name
  UCHAR    bufOwner[MAXOBJECTNAME];   // result owner
  UCHAR    bufComplim[MAXOBJECTNAME]; // result complim
  UCHAR    buf[MAXOBJECTNAME];
  int      resultType;
  int      resultLevel;
//  VNODESEL xvnode;
  REPLCONNECTPARAMS Newrepl;
  
  ZEROINIT (Newrepl);
//  ZEROINIT (xvnode);
     
  iret = DOMGetObject(hnode,
                      lpRecord->extra,            // object name
                      "",                         // no owner name
                      OT_DATABASE,                // object type
                      0,                          // level
                      NULL,                       // aparents
                      0,
                      &resultType,                // expect OT_DATABASE!
                      &resultLevel,
                      NULL,
                      objName,                    // result object name
                      bufOwner,                   // result owner
                      bufComplim);                // result complim

  if (iret != RES_SUCCESS)
    return FALSE;

     Newrepl.bLocalDb = TRUE;
     x_strcpy(Newrepl.szVNode,GetVirtNodeName (hnode));
     x_strcpy(Newrepl.szDBName,lpRecord->extra);
     x_strcpy(Newrepl.DBName,lpRecord->extra);
     x_strcpy(Newrepl.szOwner,bufOwner);

    if (TRUE /*vnoderet == OK*/) {
//      Newrepl.vnodelist = xvnode.vnodelist;
      Newrepl.nReplicVersion = ReplicVersion;
      iret = DlgAddReplConnection(hwndParent, &Newrepl);
      if (!iret)
        return FALSE;
    }

   if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndParent, buf, NULL,
                 MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
   }

  return TRUE;
}
