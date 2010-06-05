/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Visual DBA
**
**    Source : tree.c - splitted May 3, 95 on tree2.c
**    manages the lbtree inside a dom type mdi document
**
**    Author : Emmanuel Blattes
**
**  History after 04-May-1999:
**
**   06-May-1999 (schph01)
**    re-enabled the replicator "Arcclean" and "repmod" menu items
**    that had been added shortly before shipping VDBA 2.5 but
**    finally disabled at the last minute probably because of
**    QA and doc constrainsts
**   18-Jan-2000 (noifr01 and schph01)
**    (bug #100034) disabled the drag and drop functionality in the case
**    where the target object type base not a "base type" identical to the
**    source [ the object that was created was in fact a base type object
**    instead of a target type object. Example: drag and drop of a
**    grantee was creating a user (since the "properties" for a grantee, are
**    those of the user) (consistently with the right panes)
**    the only exception for this rule is drag and drop of a user into
**    "users in group" branches, where some specific code was already there
**    (a new user doesn't get created, but the user gets added to the group
**
**    uk$so01 (20-Jan-2000, Bug #100063)
**         Eliminate the undesired compiler's warning
**
**   24-Feb-2000 (schph01)
**    bug #100561 unmasked the menu "populate".
**   24-Oct-2000 (schph01)
**    SIR 102821 add menu "Comment"
**   26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**   11-Apr-2001 (schph01)
**    (sir 103292) add menu for 'usermod' command.
**   10-Dec-2001 (noifr01)
**    (sir 99596) removal of obsolete code and resources
**  18-Mar-2003 (schph01)
**   sir 107523 Add Branches 'Sequence' and 'Grantees Sequence'
**  06-Feb-2004 (schph01)
**   SIR 111752 Disable the propertie Right pane on DBEvent branch, for managing
**   DBEvent on Gateway.
**  09-Mar-2010 (drivi01)
**   SIR 123397
**   Add Refresh menu to the popup menu.
**   Add routines to refresh the tree after rollforward completes.
**  06-May-2010 (drivi01)
**   Add "Create Index" menu to the popup menu for
**   creating index for non-indexed Ingres VectorWise tables.
*****************************************************************************/

//
// Includes
//

// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "domdisp.h"
#include "dbaginfo.h"
#include "main.h"
#include "dbafile.h"
#include "dom.h"
#include "resource.h"
#include "treelb.e"   // tree listbox dll
#include "winutils.h" // icons cache
#include "msghandl.h" 
#include "dll.h"
#include "extccall.h"
#include "tree.h"

// from main.c
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);
//from dom.c
extern BOOL RefreshTrees(HWND hwndMdi, LPDOMDATA lpDomData);

//
// global variables
//

//
// Drag/Drop features
//
static int      DragDropMode;       // 0 = move, 1 = copy
static DWORD    DraggedItem;        // item from which the drag/drop started
static HWND     hwndDomDest;        // handle of destination DOM
static DWORD    ItemUnderMouse;     // tree item id in the destination tree
static HCURSOR  hCurDropY;          // Cursor when can drop
static HCURSOR  hCurDropN;          // Cursor when cannot drop
static BOOL     bOnMDIClient;       // mouse on mdi client outside any mdi doc

// from c++ source
extern void MfcBlockUpdateRightPane();
extern void MfcAuthorizeUpdateRightPane();

//
//  static functions prototypes
//
static HCURSOR NEAR IconToCursor(HINSTANCE hInst, HWND hwnd, HICON hIcon);

// dom status on global status - defined in childfrm.cpp
extern void UpdateGlobalStatusForDom(LPDOMDATA lpDomData, LPARAM lParam, LPTREERECORD lpRecord);

// Notify rightpane that current selection has changed on dom tree in left pane
// defined in childfrm.cpp
extern void TS_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear);

// Drag drop splitted functions
static void ManageDragDropStart(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData);
static void ManageDragDropMousemove(HWND hwndMdi, WPARAM wParam, LPARAM lParam);
static void ManageDragDropEnd(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData);

static void UpdateRightPaneClearFlag(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bUpdate, int initialSel, BOOL bClear)
{
  BOOL  bCurSelStatic = IsCurSelObjStatic(lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                     LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  int objType = -1;
  int CurItemBasicType = -1;
  BOOL bHasProperties = FALSE;

  // Get the base type after solve (grantee/synonymed/viewtable/alarmee
  ShowHourGlass();
  objType = SolveType(lpDomData, lpRecord, TRUE);  // NEED to update structure for star objects!
  RemoveHourGlass();
  CurItemBasicType = GetBasicType(objType);

  // Notify right pane for properties display/hide
  // properties pane - different from HasProperties4Display() since new types
  // except if lpRecord NULL, which is the case if right click in empty scratchpad
  if (lpRecord) {
    bHasProperties = HasPropertiesPane(CurItemBasicType);
    if (lpRecord->objName[0] == '\0')
      bHasProperties = FALSE;  // Special for "< No xyz >" and other < Txt > cases
    TS_MfcUpdateRightPane(hwndDoc, lpDomData, bCurSelStatic, dwCurSel, CurItemBasicType, lpRecord, bHasProperties, bUpdate, initialSel, bClear);
  }
}

void UpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bUpdate, int initialSel)
{
  UpdateRightPaneClearFlag(hwndDoc, lpDomData, bUpdate, initialSel, FALSE);
}

// for star: encapsulates IsSystemObject()
BOOL IsSystemObject2(int iobjecttype,LPUCHAR lpobjectname,LPUCHAR lpobjectowner, int parentDbType)
{
  if (iobjecttype == OT_DATABASE && parentDbType == DBTYPE_COORDINATOR)
    return TRUE;
  return IsSystemObject(iobjecttype, lpobjectname, lpobjectowner);
}


// Check whether item is on mdi client - lParam contains the mouse position
static BOOL IsCursorOnMDIClient(HWND hwndActiveDoc, LPARAM lParam)
{
  HWND      hwndCurDoc;
  LPDOMDATA lpDomData;
  RECT      refRc;
  RECT      rc;
  POINT     pt;

  POINT ptParam;

  // Parse received parameter into a point
#ifdef WIN32
    LONG2POINT(lParam, ptParam);
#else
    ptParam = MAKEPOINT(lParam);
#endif
    memcpy(&pt, &ptParam, sizeof(POINT));

  // Get rectangle of tree in active doc, then adjust point
  lpDomData = (LPDOMDATA)GetWindowLong(hwndActiveDoc, 0);
  GetWindowRect(lpDomData->hwndTreeLb, &refRc);
  pt.x += refRc.left;
  pt.y += refRc.top;

  // Check whether the mouse is outside the mdi client area
  GetWindowRect(hwndMDIClient, &rc);
  if (!PtInRect(&rc, pt))
    return FALSE;
  
  // Here mouse is inside the mdi client area.
  // scan the docs and check whether the mouse is inside a doc
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  while (hwndCurDoc) {
    // Check point placement
    GetWindowRect(hwndCurDoc, &rc);
    if (PtInRect(&rc, pt))
      return FALSE;      // inside one of the docs

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }

  // inside mdi client area, and outside any mdi doc: fine!
  return TRUE;
}

// lParam contains the mouse position
static HWND GetDomUnderCursor(HWND hwndActiveDoc, LPARAM lParam)
{
  HWND      hwndCurDoc;
  int       docType;
  LPDOMDATA lpDomData;
  RECT      refRc;
  RECT      rc;
  POINT     pt;

  POINT ptParam;

  // Parse received parameter into a point
#ifdef WIN32
  LONG2POINT(lParam, ptParam);
#else
  ptParam = MAKEPOINT(lParam);
#endif
  memcpy(&pt, &ptParam, sizeof(POINT));

  // Get rectangle of tree in active doc, then adjust point
  lpDomData = (LPDOMDATA)GetWindowLong(hwndActiveDoc, 0);
  GetWindowRect(lpDomData->hwndTreeLb, &refRc);
  pt.x += refRc.left;
  pt.y += refRc.top;

  // scan the docs and check the mouse is inside a tree
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  while (hwndCurDoc) {
    docType = QueryDocType(hwndCurDoc);
    if (docType==TYPEDOC_DOM) {
      lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
      GetWindowRect(lpDomData->hwndTreeLb, &rc);
      if (PtInRect(&rc, pt))
        return GetVdbaDocumentHandle(hwndCurDoc);
    }
    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }

  // Not found in the doms...
  return 0;
}

// lParam contains the mouse position seen from the Reference Dom
static DWORD GetTreeItemUnderMouse(HWND hwndDom, HWND hwndRefDom, LPARAM lParam)
{
  LPDOMDATA lpDomData;
  DWORD     item;
  RECT      rectDom, rectRefDom;
  POINT     pt;

  // recalculate lParam 
#ifdef WIN32
  LONG2POINT(lParam, pt);
#else
  pt=MAKEPOINT(lParam);
#endif
  GetWindowRect(hwndDom, &rectDom);
  GetWindowRect(hwndRefDom, &rectRefDom);
  pt.x = pt.x - (rectDom.left - rectRefDom.left);
  pt.y = pt.y - (rectDom.top - rectRefDom.top);
#ifdef WIN32
  // ??? POINT2LONG(pt, &lParam);  // ???
  lParam = MAKELPARAM(pt.x, pt.y);    // Low, High
#else
  lParam = MAKELPARAM(pt.x, pt.y);    // Low, High
#endif
  
  // ask for mouse item
  lpDomData = (LPDOMDATA)GetWindowLong(hwndDom, 0);
  item = SendMessage(lpDomData->hwndTreeLb, LM_GETMOUSEITEM, 0, lParam);
  return item;
}

// checks if we could drop here.
static BOOL NEAR CompatibleForDrop(HWND hwndDomDest, DWORD dwItemUnderMouse)
{
  BOOL          bStatic;
  DWORD         dwFirstChild;
  int           potAddType;
  LPTREERECORD  lpRecord;
  LPDOMDATA     lpDomData;

  lpDomData = (LPDOMDATA)GetWindowLong(hwndDomDest, 0);

  bStatic = IsItemObjStatic(lpDomData, dwItemUnderMouse);
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwItemUnderMouse);

  if (bStatic) {
    dwFirstChild = GetFirstImmediateChild(lpDomData, dwItemUnderMouse, 0);
    if (IsItemObjStatic(lpDomData, dwFirstChild))
      potAddType = GetChildType(GetItemObjType(lpDomData, dwFirstChild), FALSE);
    else
      potAddType = GetChildType(GetItemObjType(lpDomData, dwItemUnderMouse), FALSE);
  }
  else
    potAddType = lpRecord->recType;
  if (IsRelatedType(potAddType)
      || !CanObjectBeAdded(potAddType)
      || (GetBasicType(potAddType) != DomClipboardObjType) )
    return FALSE;
  if (  (GetBasicType(potAddType)!= potAddType) &&
	   !(DomClipboardObjType == OT_USER && potAddType == OT_GROUPUSER)
	 )
		return FALSE;

  // Star: restrictions for CDB as a destination
  if (lpRecord->parentDbType == DBTYPE_COORDINATOR) {
    if (potAddType == OT_TABLE || potAddType == OT_VIEW)
      return FALSE;
  }

  // fine!
  return TRUE;
}

#define IDM_TERMINATOR 0

// return value: TRUE if at least one item has been added
BOOL AddEnabledItemsToPopupMenu(HMENU hPopupMenu, HMENU hAppMenu, UINT aIdm[], BOOL bFirstSection)
{
  char  buf[BUFSIZE];
  UINT  state;
  int   cpt;
  int   arraySize;
  BOOL  bNeedSeparator;
  BOOL  bItemAdded = FALSE;

  // 1) calculate arraySize
  for (cpt=0; aIdm[cpt] != IDM_TERMINATOR; cpt++) {
    // Test non-terminated array by stopping after reasonable count
    if (cpt > 40) {
      MessageBeep(MB_ICONHAND);
      return FALSE;
    }
  }
  if(cpt == 0)
    return FALSE;    // empty array: no item added
  arraySize = cpt;

  // 2) add items that are not grayed
  for (cpt=0, bNeedSeparator = TRUE; cpt<arraySize; cpt++) {
    state = GetMenuState(hAppMenu, aIdm[cpt], MF_BYCOMMAND);
    if (state == 0xFFFFFFFF)
      continue;     // Does not exist - try next item
    if (state & MF_GRAYED)
      continue;     // Grayed : try next item

    // Here, non grayed item:
    bItemAdded = TRUE;

    // a) Add separator first if necessary
    if (!bFirstSection) {
      if (bNeedSeparator) {
        AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0);
        bNeedSeparator = FALSE;
      }
    }

    // b) add the item
    GetMenuString(hAppMenu, aIdm[cpt], buf, sizeof(buf), MF_BYCOMMAND);
    state = GetMenuState(hAppMenu, aIdm[cpt], MF_BYCOMMAND);
    AppendMenu(hPopupMenu, state | MF_STRING, aIdm[cpt], buf);
  }

  return bItemAdded;
}

//
//  Manages a popup menu that will appear when right button clicked
//
static BOOL NEAR ManagePopupMenu(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwClickedItem)
{
  POINT ptCur;
  HMENU hPopupMenu, hAppMenu;
  BOOL  bItemAdded;
  BOOL  bFirst;

  // -------- Section "AAD Mod" --------
  UINT aIdmSecAAD[] = {IDM_ADDOBJECT,
                       IDM_ALTEROBJECT,
                       IDM_DROPOBJECT,
					   IDM_CREATEIDX,
                       IDM_MODIFYSTRUCT,
                       IDM_TERMINATOR };

  // -------- Section "Grant/Revoke" --------
  UINT aIdmSecGrant[] = {IDM_GRANT,
                         IDM_REVOKE,
                         IDM_TERMINATOR };

  /* Masked   as of Oct. 10, 98   */
  /* Unmasked as of Fev. 24, 2000 */
  // -------- Section "Populate" --------
  UINT aIdmPopulate[] = {IDM_POPULATE,
                         IDM_TERMINATOR };

  // -------- Section "Star" --------
  UINT aIdmSecStar[] = {IDM_REGISTERASLINK,
                        IDM_REFRESHLINK,
                        IDM_REMOVEOBJECT,
                        IDM_TERMINATOR };

  // -------- Section "Database" --------
  UINT aIdmSecDb[] = {IDM_DUPLICATEDB,
                      IDM_INFODB,
                      IDM_CHECKPOINT ,
                      IDM_ROLLFORWARD,
                      IDM_UNLOADDB,
                      IDM_COPYDB,
                      IDM_TERMINATOR };

// -------- Section "Operations 1" --------
  UINT aIdmSecOp1[] = {IDM_GENSTAT,
                       IDM_DISPSTAT,
                       IDM_TERMINATOR };

// -------- Section "Operations 2" --------
  UINT aIdmSecOp2[] = {IDM_ALTERDB,
                       IDM_SYSMOD,
                       IDM_USERMOD,
                       IDM_AUDIT   ,
                       IDM_VERIFYDB ,
                       IDM_TERMINATOR };
// --------- Section "Refresh" --------
  UINT aIdmRefresh[] = {IDM_REFRESH,
			IDM_TERMINATOR };

  // -------- Section "Fastload" --------
  UINT aIdmFastload[] = {IDM_JOURNALING,
                         IDM_EXPIREDATE,
                         IDM_COMMENT,
                         IDM_FASTLOAD,
                         IDM_TERMINATOR };

  // -------- Section "Security Audit" --------
  UINT aIdmSecAudit[] = {IDM_SECURITYAUDIT,
                         IDM_TERMINATOR };

  // -------- Section "Database Desktop" --------
  UINT aIdmSecDbOidt[] = {IDM_LOAD,
                          IDM_UNLOAD,
                          IDM_DOWNLOAD,
                          IDM_RUNSCRIPTS,
                          IDM_UPDSTAT,
                          IDM_TERMINATOR };

  // -------- Section "Replication" --------
  UINT aIdmSecReplic[] = {IDM_REPLIC_INSTALL ,
                          IDM_REPLIC_PROPAG  ,
                          IDM_REPLIC_BUILDSRV,
                          IDM_REPLIC_CREATEKEYS,
                          IDM_REPLIC_ACTIVATE,
                          IDM_REPLIC_DEACTIVATE,
                          IDM_REPLIC_RECONCIL,
                          IDM_REPLIC_ARCCLEAN,
                          IDM_REPLIC_REPMOD,
                          IDM_REPLIC_DEREPLIC,
                          IDM_REPLIC_MOBILE,
                          IDM_TERMINATOR};

  // 1) change current selection and update menu
  SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, dwClickedItem);
  DomUpdateMenu(hwndMdi, lpDomData, TRUE);

  // 2) create pop-up menu
  GetCursorPos(&ptCur);
  hPopupMenu = CreatePopupMenu();
  if (!hPopupMenu)
    return FALSE;

  hAppMenu = hGwNoneMenu;
  if (!hAppMenu)
    return FALSE;

  // 3) fill popupmenu with enabled items only, section after section,
  // resetting bFirst only when at least one item has been added
  bFirst = TRUE;

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecAAD, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecGrant, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  /* Masked   as of Oct. 10, 98   */
  /* Unmasked as of Fev. 24, 2000 */
  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmPopulate, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecStar, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecDb, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecOp1, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecOp2, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmFastload, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecAudit, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecDbOidt, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmSecReplic, bFirst);
  bFirst = (bItemAdded ? FALSE: bFirst);

  bItemAdded =  AddEnabledItemsToPopupMenu(hPopupMenu, hAppMenu, aIdmRefresh, bFirst);
  bFirst = (bItemAdded? FALSE: bFirst);

  // 4) track the popupmenu, then destroy it
  TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                ptCur.x, ptCur.y, 0, hwndMdi, NULL);
  DestroyMenu(hPopupMenu);
  return TRUE;
}

//
// BOOL DomTreeNotifMsg(HWND hwndMdi, UINT wMsg, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam, LONG *plRet)
//
// Manages notification messages from the tree.
//
// Returns TRUE if processed, FALSE if not concerned with - MANDATORY!
//
// Also, since the LM_QUERYICONS message has been added, fills the long
// received in parameter for this message, otherwise sets it to 0L
//
// At this time, hwndMdi not used
//
BOOL DomTreeNotifMsg(HWND hwndMdi, UINT wMsg, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam, LONG *plRet)
{
  LPTREERECORD  lpRecord;
  int           ret;
  LPUCHAR       aparents[MAXPLEVEL];  // array of parent strings
  HICON         hIcon1 = 0;           // icon for static lines
  int           type;                 // line type, for icons
  int           staticType;           // ...and it's related static type

  // For Delayed right pane update management
  DWORD dwOldCurSel  = -1;
  DWORD dwNewCurSel  = -1;
  BOOL  bRButtonDown = FALSE;
  BOOL  bCommand     = FALSE;
  MSG   msg;
  LONG  lRet;

  *plRet = 0L;  // default value
  switch (wMsg) {

    // Drag and Drop messages
    case LM_NOTIFY_STARTDRAGDROP:
      ManageDragDropStart(hwndMdi, wParam, lParam, lpDomData);
      return TRUE;    // processed

    case LM_NOTIFY_DRAGDROP_MOUSEMOVE:
      ManageDragDropMousemove(hwndMdi, wParam, lParam);
      return TRUE;    // processed

    case LM_NOTIFY_ENDDRAGDROP:
      ManageDragDropEnd(hwndMdi, wParam, lParam, lpDomData);
      return TRUE;    // processed

    case LM_NOTIFY_TABKEY:
      SetFocus(lpDomData->hwndBaseOwner);
      break;

    case LM_NOTIFY_ONITEM:
      // Manage message on status bar
      if (lParam)
        lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, lParam);
      else
        lpRecord = NULL;
      UpdateGlobalStatusForDom(lpDomData, lParam, lpRecord);
      break;

    case LM_NOTIFY_SELCHANGE:
      // Update the menu bar of the dom document (grayed states)
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);
      bRButtonDown = IsRButtonDown();  // Macro in WindowsX.h
      if (bRButtonDown)
        UpdateRightPaneClearFlag(hwndMdi, lpDomData, TRUE, 0, TRUE);  // "Empty" right pane
      else {
        // store selection "before RClick"
        lpDomData->dwSelBeforeRClick = GetCurSel(lpDomData);

        UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);
      }
      break;

    case LM_NOTIFY_EXPAND:
      // special anti-reentrance-spash problem
      if (InCriticalConnectSection()) {
        //forget it - for replicator branch select & expand at the same time
        // cancel the expansion instead
        PostMessage(lpDomData->hwndTreeLb, LM_COLLAPSEBRANCH,
                    0, (LPARAM)lParam);   // lParam is the branch
        break;
      }

      // Reminder: lParam contains the record id
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, lParam);
      if (lpRecord) {
        // wParam NULL will force UpdateDisplayData (used for force refresh)
        // (if sent by the tree, wParam contains the list window handle)
        // TO BE FINISHED: FEATURE NOT USED ANYMORE - REMOVE FOR FINAL
        if ( (wParam == 0) || (lpRecord->bSubValid == FALSE)) {

          // Debug Emb 26/6/95 : performance measurement data
          #ifdef DEBUGMALLOC
          // Put a breakpoint in lbtree.cpp on ResetDebugEmbCounts()
          SendMessage(lpDomData->hwndTreeLb, LM_RESET_DEBUG_EMBCOUNTS, 0,0L);
          #endif

          // to expand, we need to refresh information
          switch (lpRecord->recType) {

            // level 0
            case OT_STATIC_DATABASE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DATABASE,                      // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

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

            case OTR_CDB:   // Star
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DATABASE_SUBSTATICS,           // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_PROFILE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_PROFILE,                       // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_USER,                          // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_GROUP:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_GROUP,                         // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_GROUP:
            case OTR_USERGROUP:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_GROUP_SUBSTATICS,              // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ROLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ROLE,                          // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_ROLE:
            case OTR_GRANTEE_ROLE:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ROLE_SUBSTATICS,               // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_LOCATION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_LOCATION,                      // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
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

            //case OT_GRANTEE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_GRANTEE_SOLVE,                 // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_USER:
            // MASQUED 7/4/95: case OT_GRANTEE_SOLVED_USER:
            case OT_GROUPUSER:
            case OT_ROLEGRANT_USER:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_USER_SUBSTATICS,               // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_SCHEMAUSER:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SCHEMAUSER_SUBSTATICS,         // iobjecttype
                      1,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // pseudo level 1 : solve the "synonymed" type
            case OT_SYNONYMOBJECT:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SYNONYMED_SOLVE,               // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1, child of database
            case OT_STATIC_R_CDB:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_CDB,                          // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLE,                         // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
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
            case OTR_LOCATIONTABLE:
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
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLE_SUBSTATICS,              // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              // parent database if related object
              // reminder: lParam contains the record id
              if (IsRelatedType(lpRecord->recType))
                AddParentBranch(lpDomData, lParam);
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_VIEW:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEW,                          // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_VIEW:
            case OTR_TABLEVIEW:
            case OTR_GRANTEE_SEL_VIEW:
            case OTR_GRANTEE_INS_VIEW:
            case OTR_GRANTEE_UPD_VIEW:
            case OTR_GRANTEE_DEL_VIEW:
            case OT_SCHEMAUSER_VIEW:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEW_SUBSTATICS,               // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              // parent database if related object
              // reminder: lParam contains the record id
              if (IsRelatedType(lpRecord->recType))
                AddParentBranch(lpDomData, lParam);
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_PROCEDURE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_PROCEDURE,                     // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEQUENCE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SEQUENCE,                      // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SCHEMA:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SCHEMAUSER,                    // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SYNONYM:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SYNONYM,                       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANTEES_SUBSTATICS,         // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_ACCESY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_ACCESY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_ACCESN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_ACCESN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CREPRY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CREPRY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CREPRN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CREPRN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CRETBY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CRETBY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CRETBN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CRETBN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_DBADMY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_DBADMY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_DBADMN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_DBADMN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_LKMODY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_LKMODY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_LKMODN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_LKMODN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYIOY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYIOY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYION_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYRWY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYRWY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYRWN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYRWN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_UPDSCY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_UPDSCY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_UPDSCN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_UPDSCN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // new section for new DB GRANTS in OpenIngres
            //

            case OT_STATIC_DBGRANTEES_SELSCY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SELSCY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_SELSCN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SELSCN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_CNCTLY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CNCTLY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CNCTLN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_CNCTLN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_IDLTLY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_IDLTLY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_IDLTLN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_IDLTLN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_SESPRY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SESPRY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_SESPRN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SESPRN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_TBLSTY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_TBLSTY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_TBLSTN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_TBLSTN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBGRANTEES_QRYCPY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYCPY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYCPN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYCPN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYPGY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYPGY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYPGN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYPGN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYCOY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYCOY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_QRYCON:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_QRYCON_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CRSEQY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SEQCRY_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;
            case OT_STATIC_DBGRANTEES_CRSEQN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBGRANT_SEQCRN_USER,           // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // end of new section for new DB GRANTS in OpenIngres
            //


            case OT_STATIC_DBEVENT:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBEVENT,                       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_REPLICATOR:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLICATOR_SUBSTATICS,         // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              if (ret == RES_SUCCESS)
                lpRecord->bSubValid = TRUE;
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBALARM_CONN_SUCCESS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;  // database name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_CO_SUCCESS_USER,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBALARM_CONN_FAILURE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;  // database name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_CO_FAILURE_USER,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBALARM_DISCONN_SUCCESS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;  // database name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_DI_SUCCESS_USER,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBALARM_DISCONN_FAILURE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;  // database name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_DI_FAILURE_USER,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1, child of group
            case OT_STATIC_GROUPUSER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_GROUPUSER,                     // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1, child of user or group or role
            case OT_STATIC_R_GRANT:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTS_SUBSTATICS,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1, child of user or group or role
            case OT_STATIC_ROLEGRANT_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ROLEGRANT_USER,                // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBEGRANT_RAISE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_RAISE_DBEVENT,        // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBEGRANT_REGISTER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_REGTR_DBEVENT,        // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_PROCGRANT_EXEC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_EXEC_PROC,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEQGRANT_NEXT:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_NEXT_SEQU,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_ACCESY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_ACCESY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_ACCESN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_ACCESN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CREPRY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CREPRY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CREPRN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CREPRN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CRETBY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CRETBY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CRETBN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CRETBN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_DBADMY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_DBADMY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_DBADMN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_DBADMN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_LKMODY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_LKMODY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_LKMODN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_LKMODN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYIOY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYIOY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYION_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYRWY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYRWY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYRWN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYRWN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_UPDSCY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_UPDSCY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_UPDSCN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_UPDSCN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // new section for new DB GRANTS in OpenIngres
            //

            case OT_STATIC_R_DBGRANT_SELSCY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SELSCY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_SELSCN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SELSCN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CNCTLY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CNCTLY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CNCTLN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_CNCTLN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_IDLTLY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_IDLTLY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_IDLTLN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_IDLTLN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_SESPRY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SESPRY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_SESPRN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SESPRN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_TBLSTY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_TBLSTY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_TBLSTN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_TBLSTN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYCPY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYCPY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYCPN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYCPN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYPGY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYPGY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYPGN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYPGN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYCOY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYCOY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_QRYCON:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_QRYCON_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // end of new section for new DB GRANTS in OpenIngres
            //

            case OT_STATIC_R_DBGRANT_CRESEQY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SEQCRY_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_DBGRANT_CRESEQN:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_DBGRANT_SEQCRN_DB,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_SEL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_SEL_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_INS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_INS_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_UPD:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_UPD_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_DEL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_DEL_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_REF:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_REF_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_CPI:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_CPI_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_CPF:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_CPF_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEGRANT_ALL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_ALL_TABLE,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_VIEWGRANT_SEL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_SEL_VIEW,             // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_VIEWGRANT_INS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_INS_VIEW,             // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_VIEWGRANT_UPD:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_UPD_VIEW,             // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_VIEWGRANT_DEL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_DEL_VIEW,             // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_ROLEGRANT:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_GRANTEE_ROLE,                 // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_USERSCHEMA:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_USERSCHEMA,                   // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_USERGROUP:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_USERGROUP,                    // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_SEL_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_SELSUCCESS_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_SEL_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_SELFAILURE_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_DEL_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_DELSUCCESS_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_DEL_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_DELFAILURE_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_INS_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_INSSUCCESS_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_INS_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_INSFAILURE_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_UPD_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_UPDSUCCESS_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_SEC_UPD_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // User's name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_ALARM_UPDFAILURE_TABLE,       // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1, child of database/replicator
            case OT_STATIC_REPLIC_CONNECTION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // base name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLIC_CONNECTION,             // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_REPLIC_CDDS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // base name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLIC_CDDS,                   // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_REPLIC_MAILUSER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // base name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLIC_MAILUSER,               // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_REPLIC_REGTABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // base name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLIC_REGTABLE,               // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "table of database"
            case OT_STATIC_INTEGRITY:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // Base name
              aparents[1] = lpRecord->extra2;   // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INTEGRITY,                     // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_RULE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_RULE,                          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_INDEX:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INDEX,                         // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_INDEX:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INDEX_SUBSTATICS,              // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_SEL_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_SEL_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_INS_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_INS_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_UPD_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_UPD_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_DEL_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_DEL_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_REF_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_REF_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_CPI_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_CPI_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_CPF_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_CPF_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLEGRANT_ALL_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_ALL_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // desktop
            case OT_STATIC_TABLEGRANT_INDEX_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_INDEX_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // desktop
            case OT_STATIC_TABLEGRANT_ALTER_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLEGRANT_ALTER_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_TABLELOCATION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_TABLELOCATION,                 // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_SEL_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_SELSUCCESS_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_SEL_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_SELFAILURE_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_DEL_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_DELSUCCESS_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_DEL_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_DELFAILURE_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_INS_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_INSSUCCESS_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_INS_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_INSFAILURE_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_UPD_SUCC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_UPDSUCCESS_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SEC_UPD_FAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_S_ALARM_UPDFAILURE_USER,       // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLESYNONYM:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_TABLESYNONYM,                 // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_TABLEVIEW:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_TABLEVIEW,                    // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_REPLIC_TABLE_CDDS:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Table name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_REPLIC_TABLE_CDDS,            // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "view of database"
            case OT_STATIC_VIEWTABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWTABLE,                     // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_VIEWGRANT_SEL_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWGRANT_SEL_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_VIEWGRANT_INS_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWGRANT_INS_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_VIEWGRANT_UPD_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWGRANT_UPD_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_VIEWGRANT_DEL_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWGRANT_DEL_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_VIEWSYNONYM:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // View name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_VIEWSYNONYM,                  // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "procedure of database"
            case OT_STATIC_PROCGRANT_EXEC_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Procedure name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_PROCGRANT_EXEC_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "Sequence of database"
            case OT_STATIC_SEQGRANT_NEXT_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Procedure name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SEQUGRANT_NEXT_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_PROC_RULE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Procedure name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_PROC_RULE,                    // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "dbevent of database"
            case OT_STATIC_DBEGRANT_RAISE_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Dbevent name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBEGRANT_RAISE_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_DBEGRANT_REGTR_USER:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // Dbevent name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_DBEGRANT_REGTR_USER,           // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 2, child of "cdds of database"
            case OT_STATIC_REPLIC_CDDS_DETAIL:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // cdds name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_REPLIC_CDDS_DETAIL,            // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_R_REPLIC_CDDS_TABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;  // cdds name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_REPLIC_CDDS_TABLE,            // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 1 - child of "location as child of table"
            case OT_TABLELOCATION:
              ret = RES_SUCCESS;
              break;

            case OT_STATIC_R_LOCATIONTABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // location name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_LOCATIONTABLE,                // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // pseudo level 2 - to determine the type of object
            // lying under a viewtable
            // i.e: is the view component a table or a view ?
            case OT_VIEWTABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;     // Base name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_VIEWTABLE_SOLVE,               // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 3, child of "index on table of database"
            case OT_STATIC_R_INDEXSYNONYM:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;   // Base name
              aparents[1] = lpRecord->extra2;    // Table name
              aparents[2] = lpRecord->extra3;    // Index name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OTR_INDEXSYNONYM,                 // iobjecttype
                      3,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            // level 3, child of "rule on table of database"
            case OT_STATIC_RULEPROC:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;    // Base name
              aparents[1] = lpRecord->extra2;   // Table name
              aparents[2] = lpRecord->extra3;   // Procedure name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_RULEPROC,                      // iobjecttype
                      3,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;


            //case OT_STATIC_DUMMY:
              break;

            // Alarms for tables and for databases
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
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ALARM_SUBSTATICS,              // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_ALARMEE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ALARMEE_SOLVE,                 // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SCHEMAUSER_TABLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;	// Database name
              aparents[1] = lpRecord->extra2;	// Schema name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SCHEMAUSER_TABLE,              // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SCHEMAUSER_VIEW:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;	// Database name
              aparents[1] = lpRecord->extra2;	// Schema name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SCHEMAUSER_VIEW,               // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_SCHEMAUSER_PROCEDURE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;	// Database name
              aparents[1] = lpRecord->extra2;	// Schema name
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_SCHEMAUSER_PROCEDURE,          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // ICE new branches
            //
            case OT_STATIC_ICE:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SUBSTATICS,                // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_SECURITY:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SECURITY_SUBSTATICS,       // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_ROLE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_ROLE,                      // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_DBUSER:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_DBUSER,                    // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_DBCONNECTION:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_DBCONNECTION,              // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_WEBUSER:
              // expansion to create objects - will also create the substatics roles and connections
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_WEBUSER,                    // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_WEBUSER_ROLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_WEBUSER_ROLE,              // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_WEBUSER_CONNECTION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_WEBUSER_CONNECTION,        // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_PROFILE:
              // expansion to create objects - will also create the substatics roles and connections
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_PROFILE,                    // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_PROFILE_ROLE:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_PROFILE_ROLE,              // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_PROFILE_CONNECTION:
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_PROFILE_CONNECTION,        // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT,                     // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_ICE_BUNIT:
              // expansion to create substatic branches
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_SUBSTATICS,          // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_SEC_ROLE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_SEC_ROLE,            // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_SEC_USER:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_SEC_USER,            // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_FACET:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_FACET,               // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_PAGE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_PAGE,                // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_FACET_ROLE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_FACET_ROLE,          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_FACET_USER:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_FACET_USER,          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_PAGE_ROLE,          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_PAGE_USER:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              aparents[1] = lpRecord->extra2;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_PAGE_USER,          // iobjecttype
                      2,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_BUNIT_LOCATION:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              aparents[0] = lpRecord->extra;
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_BUNIT_LOCATION,            // iobjecttype
                      1,                                // level
                      aparents,                         // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_SERVER:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SERVER_SUBSTATICS,         // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_SERVER_APPLICATION:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SERVER_APPLICATION,        // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_SERVER_LOCATION:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SERVER_LOCATION,           // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_ICE_SERVER_VARIABLE:
              // expansion to create objects
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_ICE_SERVER_VARIABLE,           // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            //
            // INSTALLATION LEVEL SETTINGS
            //
            case OT_STATIC_INSTALL:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INSTALL_SUBSTATICS,            // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_INSTALL_GRANTEES:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INSTALL_GRANTEES_SUBSTATICS,            // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            case OT_STATIC_INSTALL_ALARMS:
              // expansion to create static sub-items
              DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              ret = DOMUpdateDisplayData (
                      lpDomData->psDomNode->nodeHandle, // hnodestruct
                      OT_INSTALL_ALARMS_SUBSTATICS,            // iobjecttype
                      0,                                // level
                      NULL,                             // parentstrings
                      FALSE,                            // bWithChildren
                      ACT_EXPANDITEM,                   // item expanded
                      (DWORD)lParam,                    // current item
                      hwndMdi);                         // DOM MDI doc handle
              DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
              break;

            default:
              ret = RES_SUCCESS;
              //
              // Not implemented yet - Please collapse
              //
              MessageBox(GetFocus(),
                         ResourceString ((UINT)IDS_E_NOT_IMPLEMENT_COLLAPSE),
                         NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
              break;
          }

          // make missing parent branches if necessary
          // (tear-out or restart from position were made when
          // branch was not expanded yet)
          // reminder: lParam contains the record id
          if (lpRecord->parentbranch & PARENTBRANCH_NEEDED)
                AddParentBranch(lpDomData, lParam);

          // Update number of objects (necessary since we have
          // optimized the function so it does not respond if current
          // selection has been modified)
          GetNumberOfObjects(lpDomData, TRUE);
          // display will be updated at next call to the same function

          // Debug Emb 26/6/95 : performance measurement data
          #ifdef DEBUGMALLOC
          // Put a breakpoint in lbtree.cpp on ResetDebugEmbCounts()
          SendMessage(lpDomData->hwndTreeLb, LM_RESET_DEBUG_EMBCOUNTS, 0,0L);
          #endif

          // Warn the user : NOT ANYMORE
          //if (ret != RES_SUCCESS && ret != RES_TIMEOUT && ret != RES_NOGRANT)
          //  MessageBox( GetFocus(), "Error expanding item!!!",
          //              NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        }
		dwNewCurSel = GetCurSel(lpDomData);
		TS_DOMRightPaneUpdateBitmap(hwndMdi, lpDomData, dwNewCurSel, lpRecord);
      }
      break;

    case LM_QUERYICONS:
    {
      // Special management : we must return the icons handles to the caller
      LPLMQUERYICONS lpicons = (LPLMQUERYICONS)lParam;
      // changed May 12, 97: now the non-static items have custom icons!
      if (!IsItemObjStatic(lpDomData, lpicons->ulID)) {
        // changed 16/5 for fnn request: display obj type for grantees/alarmees
        type = GetItemDisplayObjType(lpDomData,lpicons->ulID);
        if (type != 0 && type != -1 /* && type != OT_STATIC_DUMMY */ ) {
          staticType = GetStaticType(type);
          ASSERT (staticType);
          if (staticType) {
            lpicons->hIcon1 = LoadIconInCache(staticType);
            lpicons->hIcon2 = (HICON)NULL;
            *plRet = 0;
          }
        }
      }

      /* old code:
      if (IsItemObjStatic(lpDomData, lpicons->ulID)) {
        type = GetItemObjType(lpDomData,lpicons->ulID);
        lpicons->hIcon1 = LoadIconInCache(type);
        lpicons->hIcon2 = (HICON)NULL;
        *plRet = 0;
      }
      */

      break;
    }

    case LM_NOTIFY_LBUTTONDBLCLK:
      // expand the item under the mouse
      SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0,
                                         (LPARAM)GetCurSel(lpDomData));
      break;

    case LM_NOTIFY_RBUTTONCLICK:

      dwOldCurSel = GetCurSel(lpDomData);
      ManagePopupMenu(hwndMdi, lpDomData, (DWORD)lParam);
      bCommand = PeekMessage(&msg, hwndMdi, WM_COMMAND, WM_COMMAND, PM_NOREMOVE);
      if (bCommand && msg.hwnd == hwndMdi) {
        PeekMessage(&msg, hwndMdi, WM_COMMAND, WM_COMMAND, PM_REMOVE);
        lRet = SendMessage(hwndMdi, WM_COMMAND, msg.wParam, msg.lParam);
        // lret can be TRUE or FALSE according to whether right pane should be updated

        dwNewCurSel = GetCurSel(lpDomData);
		
        // Now we can update right pane,
        // except if selection has been changed by the popup menu operation
        // NOTE: However, if popup menu created on another item than the currently selected one,
        // then, MUST update right pane!!!
        if (dwNewCurSel == dwOldCurSel) {
          if (lpDomData->dwSelBeforeRClick != dwNewCurSel) {
            lpDomData->dwSelBeforeRClick = dwNewCurSel;
            UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);
          }
          else
            UpdateRightPane(hwndMdi, lpDomData, (lRet? TRUE: FALSE), 0);

		  if (lRet && msg.wParam == IDM_ROLLFORWARD)
		  {
			DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, 0);
			DOMUpdateDisplayData2 (
				lpDomData->psDomNode->nodeHandle, // hnodestruct
				OT_VIRTNODE,                  // iobjecttype
				0,                            // level
				NULL,                         // parentstrings
				FALSE,                        // bWithChildren
				ACT_NO_USE,                   // not concerned :
								// forceRefresh is the key
				0L,				// no more used, all branches refreshed
								// for type combination
								// LONG/DWORD size identity assumed!
				hwndMdi,                      // current for UpdateDomData,
								// then all DOM MDI windows
								// on this node for display
				FORCE_REFRESH_ALLBRANCHES);   // Mode requested for refresh
			DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);
		  }
        }
      }
      else {
        // No command: bring back selection on item before rclick
        MfcBlockUpdateRightPane();
        SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)lpDomData->dwSelBeforeRClick);
        MfcAuthorizeUpdateRightPane();

        // Display right pane WITH THE RIGHT DETAIL PROPERTY PAGE without updating data in it
        // NOTE: detailSelBeforeRClick has been updated by mainvi2.cpp and dgdomc02.cpp
        UpdateRightPane(hwndMdi, lpDomData, FALSE, lpDomData->detailSelBeforeRClick);
      }

      break;

    case LM_NOTIFY_COLLAPSE:
	{
		LPTREERECORD  lpRecord;
		dwNewCurSel = GetCurSel(lpDomData);
		lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA, 0, (LPARAM)DraggedItem);
		TS_DOMRightPaneUpdateBitmap(hwndMdi, lpDomData, dwNewCurSel, lpRecord);
		break;
	}
    //case LM_NOTIFY_LBUTTONCLICK:
    //  break;
    //case LM_NOTIFY_RBUTTONDBLCLK:
    //  break;
    //case LM_NOTIFY_SELCHANGE:
    //  break;
    //case LM_NOTIFY_RETURNKEY:
    //  break;
    //case LM_NOTIFY_TABKEY:
    //  break;
    //case LM_NOTIFY_GETMINMAXINFOS:
    //  break;
    //case LM_NOTIFY_STRINGHASCHANGE:
    //  break;
    //case LM_NOTIFY_DESTROY:
    //  break;
    //case LM_NOTIFY_ESCKEY:
    //  break;

    default:
      return FALSE;     // Not concerned
  }
  return TRUE;    // Processed
}


//
// Transformation of icon into cursor
//
//  extracted from microsoft developer's CD (the mystery disc)
//

static HCURSOR NEAR IconToCursor(HINSTANCE hInst, HWND hwnd, HICON hIcon)
{
  auto    HCURSOR         hCursor;

  auto    HDC             hdc = 0;
  auto    HDC             hdcMonochrome = 0;
  auto    HDC             hdcWhiteBkgnd = 0;
  auto    HDC             hdcBlackBkgnd = 0;

  auto    HBITMAP         hBmpMonochrome = 0;
  auto    HBITMAP         hBmpWhiteBkgnd = 0;
  auto    HBITMAP         hBmpBlackBkgnd = 0;

  auto    HBITMAP         hBmpOrigMonochromeHdc = 0;
  auto    HBITMAP         hBmpOrigWhiteBkgndHdc = 0;
  auto    HBITMAP         hBmpOrigBlackBkgndHdc = 0;

  auto    HBITMAP         hDIBitmap = 0;

  auto    HBRUSH          hBrStockWhite = 0;
  auto    HBRUSH          hBrStockBlack = 0;

  auto    LPBITMAPINFO    lpBitmapInfo = NULL;

  auto    int             iPlanes;
  auto    int             iBitsPixel;
  auto    int             iWidth;
  auto    int             iHeight;
  auto    BITMAP          bitmap;
  auto    LPSTR           lpBitsColor;
  auto    LPSTR           lpBitsAND;
  auto    LPSTR           lpBitsXOR;
  auto    RECT            rclBitmap;

  hBrStockWhite = GetStockObject( WHITE_BRUSH );
  hBrStockBlack = GetStockObject( BLACK_BRUSH );

  hdc = GetDC( hwnd );
  if (hdc)
    {
    // Create the Memory DCs
    hdcMonochrome = CreateCompatibleDC( hdc );
    hdcWhiteBkgnd = CreateCompatibleDC( hdc );
    hdcBlackBkgnd = CreateCompatibleDC( hdc );

    if (hdcMonochrome  &&  
        hdcWhiteBkgnd  && 
        hdcBlackBkgnd)
        {
        iWidth     = GetSystemMetrics( SM_CXCURSOR );
        iHeight    = GetSystemMetrics( SM_CYCURSOR );
        iPlanes    = GetDeviceCaps( hdc, PLANES );
        iBitsPixel = GetDeviceCaps( hdc, BITSPIXEL );

        // Create the Bitmaps
        hBmpWhiteBkgnd = CreateBitmap( iWidth, iHeight,
                                      iPlanes, iBitsPixel, NULL );
        hBmpBlackBkgnd = CreateBitmap( iWidth, iHeight,
                                      iPlanes, iBitsPixel, NULL );
        hBmpMonochrome = CreateBitmap( iWidth, iHeight,
                                      1, 1, NULL );

        if (hBmpWhiteBkgnd  &&  
            hBmpBlackBkgnd  && 
            hBmpMonochrome)
          {
          // Select the Bitmaps into the DCs
          hBmpOrigWhiteBkgndHdc = SelectObject( hdcWhiteBkgnd, hBmpWhiteBkgnd );
          hBmpOrigBlackBkgndHdc = SelectObject( hdcBlackBkgnd, hBmpBlackBkgnd );
          hBmpOrigMonochromeHdc = SelectObject( hdcMonochrome, hBmpMonochrome );

          rclBitmap.left   = 0;
          rclBitmap.top    = 0;
          rclBitmap.right  = iWidth;
          rclBitmap.bottom = iHeight;

  // Step 1:
          // Create a Color Bitmap with white everywhere there is no icon
          FillRect( hdcWhiteBkgnd, &rclBitmap, hBrStockWhite );
          DrawIcon( hdcWhiteBkgnd, 0, 0, hIcon );

  // Step 2:
          // Create a Color Bitmap with black everywhere there is no icon
          FillRect( hdcBlackBkgnd, &rclBitmap, hBrStockBlack );
          DrawIcon( hdcBlackBkgnd, 0, 0, hIcon );

  // Step 3:
          // Create a Monochrome Bitmap, all white
          FillRect( hdcMonochrome, &rclBitmap, hBrStockWhite );

          GetObject( hBmpMonochrome, sizeof( BITMAP ), &bitmap );

          // Allocate memory for our AND and XOR bit masks
          lpBitsAND = (LPSTR)ESL_AllocMem( bitmap.bmPlanes *
                                      bitmap.bmHeight *
                                      bitmap.bmWidthBytes );

          lpBitsXOR = (LPSTR)ESL_AllocMem( bitmap.bmPlanes *
                                      bitmap.bmHeight *
                                      bitmap.bmWidthBytes );

          if (lpBitsXOR  &&  lpBitsAND)
              {
              // Set background/foreground  colors for automatic conversion
              // by windows on color to monochrome bitmap.
              // Background color goes to white, all others go to black

              SetBkColor  ( hdcWhiteBkgnd, RGB( 255,255,255 ) );
              SetTextColor( hdcWhiteBkgnd, RGB( 0, 0, 0 ) );

              SetBkColor  ( hdcBlackBkgnd, RGB( 255,255,255 ) );
              SetTextColor( hdcBlackBkgnd, RGB( 0, 0, 0 ) );

  // Step 4:
              // BitBlit the Color Bitmat w/Icon and White Background onto
              // Monochrome White Bitmap using the AND raster operation
              BitBlt( hdcMonochrome, 0, 0, iWidth, iHeight, 
                      hdcWhiteBkgnd, 0, 0, SRCAND );

  // Step 5:
              // BitBlit the Color Bitmat w/Icon and Black Background onto
              // Monochrome Bitmap just used as destination using the XOR
              // raster operation
              BitBlt( hdcMonochrome, 0, 0, iWidth, iHeight, 
                      hdcBlackBkgnd, 0, 0, SRCINVERT );

  // Step 6:
              // Retrieve the bits for our AND bit mask
              GetBitmapBits( hBmpMonochrome, bitmap.bmPlanes *
                                            bitmap.bmHeight *
                                            bitmap.bmWidthBytes, lpBitsAND );

  // Step 7:
              // Create a Monochrome Bitmap, all black
              FillRect( hdcMonochrome, &rclBitmap, hBrStockBlack );

              GetObject( hBmpWhiteBkgnd, sizeof( BITMAP ), &bitmap );

              // Set up/create a DIB
              lpBitsColor  = (LPSTR)ESL_AllocMem( bitmap.bmPlanes *
                                            bitmap.bmHeight *
                                            bitmap.bmWidthBytes );
              if (lpBitsColor)
                {
                lpBitmapInfo = (LPBITMAPINFO)ESL_AllocMem( sizeof( BITMAPINFOHEADER ) +
                                                    ( sizeof( RGBQUAD ) * 16 ) );

                if (lpBitmapInfo)
                    {
                    _fmemset( lpBitmapInfo, 0, sizeof( BITMAPINFOHEADER ) +
                                            ( sizeof( RGBQUAD ) * 16 ) );

                    lpBitmapInfo->bmiHeader.biSize          =
                                                        sizeof( BITMAPINFOHEADER );
                    lpBitmapInfo->bmiHeader.biWidth         = iWidth;
                    lpBitmapInfo->bmiHeader.biHeight        = iHeight;
                    lpBitmapInfo->bmiHeader.biPlanes        = 1;
                    lpBitmapInfo->bmiHeader.biBitCount      = bitmap.bmPlanes;
                    lpBitmapInfo->bmiHeader.biCompression   = BI_RGB;
                    lpBitmapInfo->bmiHeader.biSizeImage     = 0;
                                                      // pixels/inch * inch/meter
                    lpBitmapInfo->bmiHeader.biXPelsPerMeter =
                          GetDeviceCaps( hdcMonochrome, LOGPIXELSX ) * 394 / 10;
                    lpBitmapInfo->bmiHeader.biYPelsPerMeter =
                          GetDeviceCaps( hdcMonochrome, LOGPIXELSY ) * 394 / 10;
                    lpBitmapInfo->bmiHeader.biClrUsed       = 0;
                    lpBitmapInfo->bmiHeader.biClrImportant  = 0;

                    // Retrieve the bits (in DIB format) from our Color
                    // Bitmap with an Icon and Black background
                    GetDIBits( hdcMonochrome, hBmpBlackBkgnd, 0,
                              bitmap.bmHeight, lpBitsColor,
                              lpBitmapInfo, DIB_RGB_COLORS );

  // Step 8:
                    // Create a DIB from the bits just retrieved
                    hDIBitmap = CreateDIBitmap( hdcMonochrome,
                                                (LPBITMAPINFOHEADER)lpBitmapInfo,
                                                CBM_INIT, lpBitsColor,
                                                lpBitmapInfo, DIB_RGB_COLORS );
                    if (hDIBitmap)
                      {
                      SelectObject( hdcBlackBkgnd, hDIBitmap );

  // Step 9:
                      // BitBlit the Color DIB w/Icon and Black Background
                      // onto Monochrome Black Bitmap using the OR raster
                      // operation
                      BitBlt( hdcMonochrome, 0, 0, iWidth, iHeight,
                              hdcBlackBkgnd, 0, 0, SRCPAINT );

                      // Retrieve the bits for our XOR bit mask
                      GetBitmapBits( hBmpMonochrome, bitmap.bmPlanes *
                                                      bitmap.bmHeight *
                                                      bitmap.bmWidthBytes,
                                                      lpBitsXOR );

                      // Create the Cursor from AND and XOR bit mask
                      // with the hot spot being the center of the cursor
                      hCursor = CreateCursor( hInst, iWidth / 2, iHeight / 2,
                                                      iWidth, iHeight,
                                                      lpBitsAND, lpBitsXOR );

                      // Restore and cleanup the DIB
                      SelectObject( hdcBlackBkgnd, hBmpOrigBlackBkgndHdc );
                      DeleteObject( hDIBitmap );
                      }
                    else  // hDIBitmap = = 0
                      hCursor = 0;

                    ESL_FreeMem( lpBitmapInfo );
                    }
                else  // ESL_AllocMem for lpBitmapInfo = = NULL
                    hCursor = 0;

                ESL_FreeMem( lpBitsColor );
                }
              else  // ESL_AllocMem for lpBitsColor = = NULL
                hCursor = 0;
              }
          else  // lpBitsXOR and/or lpBitsAND NULL
              hCursor = 0;

          if (lpBitsXOR)
              ESL_FreeMem( lpBitsXOR );
          if (lpBitsAND )
              ESL_FreeMem( lpBitsAND );

          // Reselect the original BITMAPS into the DCs so our
          // Created Bitmaps can be deleted
          SelectObject( hdcWhiteBkgnd, hBmpOrigWhiteBkgndHdc );
          SelectObject( hdcBlackBkgnd, hBmpOrigBlackBkgndHdc );
          SelectObject( hdcMonochrome, hBmpOrigMonochromeHdc );
          }
        else  // one/two/all of hBmpWhiteBkgnd, hBmpBlackBkgnd, hBmpMonochrome 0
          hCursor = 0;

        // Delete the bitmaps
        if (hBmpMonochrome)
          DeleteObject( hBmpMonochrome );
        if (hBmpWhiteBkgnd)
          DeleteObject( hBmpWhiteBkgnd );
        if (hBmpBlackBkgnd)
          DeleteObject( hBmpBlackBkgnd );
        }
    else  // one/two/all of hdcMonochrome ,hdcWhiteBkgnd, hdcBlackBkgnd 0
        hCursor = 0;

    // Delete the Memory DCs
    if (hdcMonochrome)
        DeleteDC( hdcMonochrome );
    if (hdcWhiteBkgnd)
        DeleteDC( hdcWhiteBkgnd );
    if (hdcBlackBkgnd)
        DeleteDC( hdcBlackBkgnd );

    ReleaseDC( hwnd, hdc );
    }
  else
    hCursor = 0;

  return hCursor;
}


// properties pane - different from HasProperties4Display() since new types
BOOL HasPropertiesPane(int objType)
{
  if (HasProperties4display(objType))
    return TRUE;

  //
  // Types that didn't have old-style properties
  //
  switch (objType) {
    //
    // objects
    //
    case OT_DBEVENT:
    {
    /* Dbevents are available on Gateway Oracle, but  without propertie pane */
        if (GetOIVers() == OIVERS_NOTOI)
            break;
    }
    case OT_REPLIC_CDDS:

    //
    // static branches
    //
    case OT_STATIC_REPLICATOR:
    case OT_STATIC_LOCATION:

//#define MASK_PROPERTIES_4_STATIC_ITEMS
#ifndef MASK_PROPERTIES_4_STATIC_ITEMS

    // database
    case OT_STATIC_DBGRANTEES:
    case OT_STATIC_DBALARM:

    // table
    case OT_STATIC_SECURITY:
    case OT_STATIC_TABLEGRANTEES:

    // view
    case OT_STATIC_VIEWGRANTEES:

    // dbevent: useless since immediate accessibility

    // procedure - grantees - of limited interest
    case OT_STATIC_PROCGRANT_EXEC_USER:
    case OT_STATIC_SEQGRANT_NEXT_USER:

#endif MASK_PROPERTIES_4_STATIC_ITEMS

    case OT_REPLIC_CONNECTION:

    case OT_SCHEMAUSER:
    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:

    // enhancement: pages on most static items
    case OT_STATIC_TABLE:
    case OT_STATIC_VIEW:
    case OT_STATIC_PROCEDURE:
    case OT_STATIC_SEQUENCE:
    case OT_STATIC_SCHEMA:
    case OT_STATIC_SYNONYM:
    case OT_STATIC_DBEVENT:

    case OT_STATIC_INTEGRITY:
    case OT_STATIC_RULE:
    case OT_STATIC_INDEX:
    case OT_STATIC_TABLELOCATION:
    case OT_STATIC_GROUPUSER:

    case OT_STATIC_DATABASE:
    case OT_STATIC_PROFILE:
    case OT_STATIC_USER:
    case OT_STATIC_GROUP:
    case OT_STATIC_ROLE:

    case OT_ICE_DBCONNECTION:
    case OT_ICE_DBUSER:
    case OT_ICE_PROFILE:
    case OT_ICE_ROLE:
    case OT_ICE_WEBUSER:
    case OT_ICE_WEBUSER_CONNECTION:
    case OT_ICE_PROFILE_CONNECTION:
    case OT_ICE_WEBUSER_ROLE:
    case OT_ICE_PROFILE_ROLE:
    case OT_ICE_BUNIT_LOCATION:
    case OT_ICE_SERVER_LOCATION:
    case OT_ICE_SERVER_VARIABLE:
    case OT_ICE_BUNIT:
    case OT_STATIC_ICE_SECURITY:
    case OT_STATIC_ICE_ROLE:
    case OT_STATIC_ICE_DBUSER:
    case OT_STATIC_ICE_DBCONNECTION:
    case OT_STATIC_ICE_WEBUSER:
    case OT_STATIC_ICE_PROFILE:
    case OT_STATIC_ICE_WEBUSER_ROLE:
    case OT_STATIC_ICE_WEBUSER_CONNECTION:
    case OT_STATIC_ICE_PROFILE_ROLE:
    case OT_STATIC_ICE_PROFILE_CONNECTION:
    case OT_STATIC_ICE_SERVER:
    case OT_STATIC_ICE_SERVER_APPLICATION:
    case OT_STATIC_ICE_SERVER_LOCATION:
    case OT_STATIC_ICE_SERVER_VARIABLE:
    case OT_STATIC_INSTALL_SECURITY:
    case OT_STATIC_INSTALL_GRANTEES:
    case OT_STATIC_INSTALL_ALARMS:
    case OT_ICE_BUNIT_FACET:
    case OT_ICE_BUNIT_PAGE:

    case OT_ICE_BUNIT_FACET_ROLE:
    case OT_ICE_BUNIT_FACET_USER:
    case OT_ICE_BUNIT_PAGE_ROLE:
    case OT_ICE_BUNIT_PAGE_USER:

    case OT_ICE_BUNIT_SEC_ROLE:
    case OT_ICE_BUNIT_SEC_USER:

    case OT_STATIC_ICE_BUNIT:
    case OT_STATIC_ICE_BUNIT_FACET:
    case OT_STATIC_ICE_BUNIT_PAGE:
    case OT_STATIC_ICE_BUNIT_LOCATION:

    case OT_STATIC_ICE_BUNIT_SEC_ROLE:
    case OT_STATIC_ICE_BUNIT_SEC_USER:

    case OT_STATIC_ICE_BUNIT_FACET_ROLE:
    case OT_STATIC_ICE_BUNIT_FACET_USER:
    case OT_STATIC_ICE_BUNIT_PAGE_ROLE:
    case OT_STATIC_ICE_BUNIT_PAGE_USER:

      return TRUE;

    case OT_PROCEDURE:
    case OT_RULE:
      break;
  }
  return FALSE;
}

//
// Drag drop splitted functions
//

static void ManageDragDropStart(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData)
{
  HICON         hIcon;                // intermediate for drag/drop cursor
  int           itemType;             // drag/drop candidate item type...
  BOOL          bCancelDragDrop;
  int           staticType;           // ...and it's related static type

  bOnMDIClient = FALSE;       // default to not on mdi client

  DraggedItem = lParam;

  // Fnn/John request: always copy, no need for ctrl to be depressed anymore
  DragDropMode = 1;   // copy
  // old code, for backward compatibility: DragDropMode = wParam;    // 0 = move, 1 = copy

  itemType = GetItemObjType(lpDomData, DraggedItem);

  // check that the start object can be either moved or copied
  // TO BE FINISHED FOR MOVE - AT THIS TIME, WE NEVER ACCEPT
  bCancelDragDrop = FALSE;
  if (DragDropMode == 0)
    bCancelDragDrop = TRUE;
  else {
    if (IsItemObjStatic(lpDomData, DraggedItem))
      bCancelDragDrop = TRUE;
    else {
      LPTREERECORD  lpRecord;
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)DraggedItem);
      if (!CanObjectBeCopied(itemType, lpRecord->objName, lpRecord->ownerName))
        bCancelDragDrop = TRUE;

      // Star: restrictions for DDB as a source
      if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
        bCancelDragDrop = TRUE;

      // Star: restrictions for CDB as a source
      if (lpRecord->parentDbType == DBTYPE_COORDINATOR) {
        if (itemType == OT_TABLE || itemType == OT_VIEW || itemType == OT_PROCEDURE)
          bCancelDragDrop = TRUE;
      }
    }
  }
  // Restricted features if gateway
  //if (GetOIVers() == OIVERS_NOTOI)
  //  bCancelDragDrop = TRUE;

  if (bCancelDragDrop) {
    SendMessage(lpDomData->hwndTreeLb, LM_CANCELDRAGDROP, 0, 0L);
    DraggedItem = 0;
    DragDropMode = 0;
    return;
  }

  // Set the capture on the current tree
  SetCapture(lpDomData->hwndTreeLb);

  // Load the right cursors
  if (DragDropMode==0) {
    hCurDropY = LoadCursor(NULL, IDC_CROSS); //LoadCursor(hResource, MAKEINTRESOURCE(IDC_DOM_MV_CANDROP));
    hCurDropN = LoadCursor(NULL, IDC_NO); //LoadCursor(hResource, MAKEINTRESOURCE(IDC_DOM_MV_CANNOTDROP));
  }
  else {
    // Build cursor from icon
    hCurDropY = 0;
    staticType = GetStaticType(itemType);
    if (staticType) {
      hIcon = LoadIconInCache(staticType);
      if (hIcon)
        hCurDropY = IconToCursor(hResource, hwndMdi, hIcon);
    }
    if (!hCurDropY)
      hCurDropY = LoadCursor(NULL, IDC_CROSS); //LoadCursor(hResource, MAKEINTRESOURCE(IDC_DOM_CP_CANDROP));
    hCurDropN = LoadCursor(NULL, IDC_NO);//LoadCursor(hResource, MAKEINTRESOURCE(IDC_DOM_CP_CANNOTDROP));
  }
  SetCursor(hCurDropY);

  // Set DomClipboardObjType variable for drop authorization management
  CopyDomObjectToClipboard(hwndMdi, lpDomData, TRUE);
}

static BOOL DragDropMousemove(HWND hwndMdi, WPARAM wParam, LPARAM lParam, BOOL bRightPane);

BOOL RPaneDragDropMousemove(HWND hwndMdi, int x, int y)
{
  WPARAM wParam = 0;
  LPARAM lParam = 0;

  // Get rectangle of tree in active doc, and adjust x/y
  RECT refRc;
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0); // GetVdbaDocumentHandle not needed
  GetWindowRect(lpDomData->hwndTreeLb, &refRc);
  x -= refRc.left;
  y -= refRc.top;
  lParam = MAKELPARAM(x, y);    // Low, High

  return DragDropMousemove(hwndMdi, wParam, lParam, TRUE);
}

static void ManageDragDropMousemove(HWND hwndMdi, WPARAM wParam, LPARAM lParam)
{
  DragDropMousemove(hwndMdi, wParam, lParam, FALSE);
}

static BOOL DragDropMousemove(HWND hwndMdi, WPARAM wParam, LPARAM lParam, BOOL bRightPane)
{
  // Clipboard
  LPDOMDATA     lpDestDomData;

  // manage the cursor

  // Added Emb Sep. 9, 95 : we can receive dragdrop_mousemove
  // when the "table populate" message box is displayed
  if (!bRightPane) {
    if (hCurDropY == NULL || hCurDropN == NULL)
      return FALSE;
  }

  // Check whether item is on mdi client - always reset/ignore if from right pane
  if (bRightPane)
    bOnMDIClient = FALSE;
  else
    bOnMDIClient = IsCursorOnMDIClient(hwndMdi, lParam);
  if (bOnMDIClient) {
    if (bRightPane)
      return TRUE;
    else
      SetCursor(hCurDropY);
  }
  else {
    // get item under mouse (0 means inter-node not acceptable)
    ItemUnderMouse = 0L;    // defaults to not acceptable
    hwndDomDest = GetDomUnderCursor(hwndMdi, lParam);
    if (hwndDomDest) {
      lpDestDomData = (LPDOMDATA)GetWindowLong(hwndDomDest, 0);
      ItemUnderMouse = GetTreeItemUnderMouse(hwndDomDest, hwndMdi, lParam);
    }

    if (hwndDomDest && ItemUnderMouse)
      if (CompatibleForDrop(hwndDomDest, ItemUnderMouse))
        if (bRightPane)
          return TRUE;
        else
          SetCursor(hCurDropY);
      else {
        ItemUnderMouse = 0L;  // So enddragdrop will not attempt to drop
        if (bRightPane)
          return FALSE;
        else
          SetCursor(hCurDropN);
      }
    else 
      if (bRightPane)
        return FALSE;
      else
        SetCursor(hCurDropN);
  }
  return FALSE;
}


static BOOL DragDropEnd(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord);

BOOL RPaneDragDropEnd(HWND hwndMdi, int x, int y, LPTREERECORD lpRightPaneRecord)
{
  WPARAM wParam = 0;
  LPARAM lParam = 0;

  // Get rectangle of tree in active doc, and adjust x/y
  RECT refRc;
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0); // GetVdbaDocumentHandle not needed
  GetWindowRect(lpDomData->hwndTreeLb, &refRc);
  x -= refRc.left;
  y -= refRc.top;
  lParam = MAKELPARAM(x, y);    // Low, High

  return DragDropEnd(hwndMdi, wParam, lParam, lpDomData, TRUE, lpRightPaneRecord);
}

static void ManageDragDropEnd(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData)
{
  DragDropEnd(hwndMdi, wParam, lParam, lpDomData, FALSE, 0);
}

int StandardDragDropEnd(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData)
{
  return Task_StandardDragDropEndInterruptible(hwndMdi, lpDomData, bRightPane, lpRightPaneRecord, hwndDomDest, lpDestDomData);
}

int StandardDragDropEnd_MT(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord, HWND hwndDomDest, LPDOMDATA lpDestDomData)
{
  BOOL bCopyOK = FALSE;

  //ShowHourGlass();
  ClearDomObjectFromClipboard(hwndMdi);

  if (HasLoopInterruptBeenRequested())
    return RES_ERR;

  if (bRightPane)
    bCopyOK = CopyRPaneObjectToClipboard(hwndMdi, DomClipboardObjType, FALSE, lpRightPaneRecord);
  else
    bCopyOK = CopyDomObjectToClipboard(hwndMdi, lpDomData, FALSE);

  if (HasLoopInterruptBeenRequested())
    return RES_ERR;

  if (bCopyOK) {
    DWORD dwOldDestSel;
    BOOL bPaste = FALSE;
    lpDestDomData = (LPDOMDATA)GetWindowLong(hwndDomDest, 0);

    // Had been masqued not to refresh right pane in destination dom, but is MANDATORY!
    // unmasqued, but mask right pane update
    MfcBlockUpdateRightPane();
    dwOldDestSel = GetCurSel(lpDestDomData);
    SendMessage(lpDestDomData->hwndTreeLb, LM_SETSEL, 0, ItemUnderMouse);
    MfcAuthorizeUpdateRightPane();
    UpdateRightPaneClearFlag(hwndDomDest, lpDestDomData, TRUE, 0, TRUE);  // "Empty" right pane

    if (HasLoopInterruptBeenRequested())
      return RES_ERR;

    MfcBlockUpdateRightPane();
    bPaste = PasteDomObjectFromClipboard(hwndDomDest, lpDestDomData, FALSE);
    MfcAuthorizeUpdateRightPane();

    if (HasLoopInterruptBeenRequested())
      return RES_ERR;

    if (!bPaste) {
      //RemoveHourGlass();
      return RES_ERR;
    }
    // Delete object in current dom if it was a move
    if (DragDropMode == 0) { // move
      DomDropObject(hwndMdi, lpDomData, TRUE, 0L); // Don't ask confirm
      if (HasLoopInterruptBeenRequested())
        return RES_ERR;
    }

    // Change active document
    if (hwndDomDest != hwndMdi)
      SendMessage(hwndMDIClient, WM_MDIACTIVATE, (WPARAM)GetParent(GetParent(hwndDomDest)), 0L);

    if (HasLoopInterruptBeenRequested())
      return RES_ERR;
  }
  else {
   // Refresh right pane of destination dom, WHICH IS NOT THE ACTIVE DOCUMENT!
   int oldOiVers = SetOIVers (lpDestDomData->ingresVer);
   UpdateRightPane(hwndDomDest, lpDestDomData, TRUE, 0);
   SetOIVers(oldOiVers);
   return RES_ERR;    // since copy has failed
  }
  //RemoveHourGlass();
  return RES_SUCCESS;
}

static BOOL DragDropEnd(HWND hwndMdi, WPARAM wParam, LPARAM lParam, LPDOMDATA lpDomData, BOOL bRightPane, LPTREERECORD lpRightPaneRecord)
{
  char          rs[BUFSIZE];
  char          buf[BUFSIZE];
  LPDOMDATA     lpDestDomData;
  LPTREERECORD  lpRecord;

  // NOTE : if bRightPane is set, DraggedItem value is 0xFFFFFFFF!

  // star ddb management
  BOOL  bDragToDDB = FALSE;
  BOOL  bCopyOK    = FALSE;

      if (!bRightPane) {
        ReleaseCapture();
        SetCursor(LoadCursor(0, IDC_ARROW));
      }
      // Emb July 7, 97: don't destroy if null cursor
      if (hCurDropY)
        DestroyCursor(hCurDropY);
      if (hCurDropN)
        DestroyCursor(hCurDropN);

      // Added Emb Sep. 9, 95 : we can receive dragdrop_mousemove
      // when the "table populate" message box is displayed
      hCurDropY = hCurDropN = NULL;

      // Emb July 7, 97: no action if drag mode not started (item could not be dragged: static, system object, ...)
      // bug fix: we bumped into a gpf in next wsprintf since GetObjName() returned NULL
      if (!DraggedItem)
        return FALSE;

      // New as of Oct 10, 98 : if target is MDIClient, act as Tear Out
      if (bOnMDIClient) {
        // FINIR : QUID SI DRAG DEPUIS RPANE ???
        MakeNewDomMDIChild (DOM_CREATE_TEAROUT, hwndMdi, 0, 0);
        return TRUE;
      }

      // no action if last mouse position unacceptable
      if (!hwndDomDest || !ItemUnderMouse)
        return FALSE;

      // nothing to do if destination idem source!
      if (hwndDomDest == hwndMdi && ItemUnderMouse == DraggedItem )
        return FALSE;

      // get destination dom data 
      lpDestDomData = (LPDOMDATA)GetWindowLong(hwndDomDest, 0);

      // Ask user for confirmation - special management for "register as link" if destination is ddb
      lpRecord = (LPTREERECORD) SendMessage(lpDestDomData->hwndTreeLb,
                                            LM_GETITEMDATA, 0, (LPARAM)ItemUnderMouse);
      if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
        bDragToDDB = TRUE;
      else
        bDragToDDB = FALSE;

      if (DragDropMode==0)
        LoadString(hResource, IDS_CONFIRM_MV_DRAGDROP, rs, sizeof(rs));
      else {
        if (bDragToDDB)
          LoadString(hResource, IDS_CONFIRM_DRAGTODDB, rs, sizeof(rs));
        else
          LoadString(hResource, IDS_CONFIRM_CP_DRAGDROP, rs, sizeof(rs));
      }
      if (bRightPane)
        wsprintf(buf, rs, lpRightPaneRecord->objName);
      else
        wsprintf(buf, rs, GetObjName(lpDomData, DraggedItem));
      LoadString(hResource, IDS_CONFIRM_DRAGDROP_CAPT, rs, sizeof(rs));
      if (MessageBox(hwndMdi, buf, rs,
                     MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) != IDYES)
        return FALSE;

      //
      // Special management for drag-drop to ddb, i.e. "Register As Link"
      //
      if (bDragToDDB) {
        DomRegisterAsLink(hwndMdi, lpDomData, TRUE, hwndDomDest, lpDestDomData, ItemUnderMouse);
        return FALSE;
      }

      //
      //  Standard drag-drop through clipboard
      //
      {
        int res = StandardDragDropEnd(hwndMdi, lpDomData, bRightPane, lpRightPaneRecord, hwndDomDest, lpDestDomData);

        // IN THE MAIN THREAD: Refresh right pane of destination dom, WHICH HAS BECOME THE ACTIVE DOCUMENT!
        UpdateRightPane(hwndDomDest, lpDestDomData, TRUE, 0);

        if (res != RES_SUCCESS) {
          /* Can't tell whether interrupted or failed
            MessageBox(hwndMdi, "Drag-Drop operation Interrupted by user Action", "Drag-Drop", MB_ICONEXCLAMATION | MB_OK);
          */
          return FALSE;
        }
      }
      return TRUE;
}

BOOL ManageDragDropStartFromRPane(HWND hwndMdi, int rPaneObjType, LPUCHAR objName, LPUCHAR ownerName)
{
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  if (!lpDomData)
    return FALSE;

  if (!CanObjectBeCopied(rPaneObjType, objName, ownerName))
    return FALSE;

  bOnMDIClient = FALSE;       // default to not on mdi client

  DraggedItem = (DWORD)(-1);

  // Fnn/John request: always copy, no need for ctrl to be depressed anymore
  DragDropMode = 1;   // copy
  // old code, for backward compatibility: DragDropMode = wParam;    // 0 = move, 1 = copy

  if (DragDropMode == 0)
    return FALSE;

  // Set DomClipboardObjType variable for drop authorization management
  if (!CopyRPaneObjectToClipboard(hwndMdi, rPaneObjType, TRUE, NULL))
    return FALSE;

  return TRUE;
}

