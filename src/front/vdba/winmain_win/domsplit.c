/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Project : CA/OpenIngres Visual DBA
**
**    Source : domsplit.c
**    manage a DOM document
**        splitted from dom.c May 5, 95
**
**    Author : Emmanuel Blattes
**
**  History after 04-May-1999:
**
**   11-Jan-2000 (schph01)
**    Bug #99966
**    Copy the schema of the table in field :
**             LPTABLEPARAMS->STORAGEPARAMS.objectowner.
**    21-01-2000 (schph01)
**     (bug 100107)  test if the columns are of type UDTs,or spatial
**     data type, and display a message and stop the Drag and Drop
**     functionality.
**
**    25-01-2000 (schph01)
**     Bug #97680 Additional change, Remove display quote before call
**     GetDetailInfo() function.
**
**    01-Mar-2000 (schph01)
**     bug #100666 Verify before launched the populate functionality that the
**     II_DATE_FORMAT variable is know.
**    24-Oct-2000 (schph01)
**     SIR 102821 Manage new menu "Comment"
**    26-Mar-2001 (noifr01)
**      (sir 104270) removal of code for managing Ingres/Desktop
**    30-Mar-2001 (noifr01)
**       (sir 104378) differentiation of II 2.6.
**    11-Apr-2001 (schph01)
**       (sir 103292) manage new menu "usermod".
**    26-Apr-2001 (schph01)
**       (sir 103299) launch new "copydb" dialog box for Ingres 2.6 version
** 02-May-2001 (uk$so01)
**    SIR #102678
**    Support the composite histogram of base table and secondary index.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 09-Jul-2001 (hanje04)
**    Don't include ansiapi.h for MAINWIN. Not supported in 4.02
** 08-Oct-2001 (schph01)
**    SIR 105881 add function VerifyNumKeyColumn() verify the number of
**    columns in the current index.
** 18-Oct-2001 (schph01)
**    bug 106086 Add bVdbaHelpEnabled variable managed by SetDisplayVdbaHelp()
**    and GetVdbaHelpStatus() functions.
** 10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
** 07-Mar-2002 (noifr01)
**    (bug 107135) when updating subbranches recursively, update also
**    sub-branches in the cache (which may be present because of a
**    right pane, even if the corresponding branches have not been
**    expanded in the tree) within the loop (multiple updates will
**    normally be avoided thanks to the bUnique flag and the previous 
**    call of the PrepareGlobalDOMUpd() function)
** 13-Dec-2002 (schph01)
**    bug 109309 Perform a lowercase comparison between the current user name
**    and 'ingres' name to launched the sysmod command.
** 14-Jan-2003 (schph01)
**    SIR 109430 When the columns are unknown type a message is
**    displayed and the Drag and Drop functionality is stopped.
** 18-Mar-2003 (schph01)
**    sir 107523 Add Branches 'Sequence' and 'Grantees Sequence'
** 27-Mar-2003 (noifr01)
**    (bug 109837) disable functionality against older versions of Ingres,
**    that would require to update rmcmd on these platforms
** 23-Apr-2003 (schph01)
**    bug 109832 before launch the sysmod command check if the current owner
**    of the Rmcmd Object is the same that the owner of the vnode.
**    Additional change for Sysmod and AlterDB :
**       - Add wait cursor before launch GetRmcmdObjects_Owner() function
**       - Display message if GetRmcmdObjects_Owner() function failed
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 12-Sep-2003 (schph01)
**    bug 110664 before launch the ckpdb, RollforwardDB and AuditDB command
**    check if the current owner of Rmcmd  object is the same that the
**    owner of the vnode.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
**  22-Oct-2003 (schph01)
**  (SIR 111153) When the Ingres version is 2.65 or higher:
**  Used function MfcGetRmcmdObjects_launcher() to retrieve the user was 
**  launched the rmcmd process.
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 03-Feb-2004 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX or COM:
**    Integrate Export assistant into vdba
** 05-Aug-2004 (uk$so01)
**    SIR 111014 (build GUI tools in new build environment)
**    Remove the #include <ansiapi.h> and replace A2W by CMmbstowcs
** 02-apr-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, clean up warnings.
** 12-May-2010 (drivi01)
**    Disable menus for Ingres VectorWise tables for unsupported
**    features.
** 02-Jun-2010 (drivi01)
**    Remove hardcoded buffer sizes.
*/

//
// Find and Close the open cursor:
#define _FINDnCLOSE_OPENCURSOR 

//
// Includes
//
// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "domdisp.h"
#include "dbaginfo.h"
#include "error.h"
#include "main.h"
#include "dom.h"
#include "resource.h"
#include "winutils.h"
#include "treelb.e"   // tree list dll
#include "nanact.e"   // nanbar and status bar dll
#include "tree.h"     // uses UCHAR, so needs to be after dba.h
// for print functions in print.c
#include "typedefs.h"
#include "stddlg.e"
#include "print.e"
#include "msghandl.h"
#include "dll.h"
// for dialogs ids used by context help system
#include "dlgres.h"
#include "monitor.h"
#include "extccall.h"
#include "DgErrH.h"

#include "libextnc.h"

static BOOL RemoteOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void RemoteOnDestroy(HWND hDlg);
static void RemoteOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void RemoteOnTimer(HWND hDlg, UINT id);
static BOOL PilotOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void PilotOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void PilotOnDestroy(HWND hwnd);
extern BOOL MfcGetRmcmdObjects_launcher ( char *VnodeName, char *objName);
//
// Local variables for clipboard - some of them made public because 
// of DomUpdateMenu cut into pieces in OnUpdateXyz() handles of CChildFrame
//
static int DomClipboardLastStoredType;  // duplicate DomClipboardObjType
static int DomClipboardObjOIVers;
static int DomClipboardSrcNodeHandle;   // for drag-drop ingres to desktop

static BOOL bVdbaHelpEnabled = TRUE;

BOOL GetVdbaHelpStatus ()
{
  return bVdbaHelpEnabled;
}

static void SetDisplayVdbaHelp (BOOL bVal)
{
  bVdbaHelpEnabled = bVal;
}

// prototyped due to < no database > management for menuitems deactivation
static BOOL NEAR IsNoItem(int objType, char *objName);

// special added 5/5/97 to force update of replicator-related items
// after operation on replicator
BOOL DomGetMenuItemState(HWND hWnd, UINT itemId);
extern void MfcClearReplicRightPane(HWND hWnd, LPTREERECORD lpRecord);   // from childfrm.cpp
void SetReplicatorRequestMandatory(HWND hWnd, LPDOMDATA lpDomData)
{
  // "Empty" right pane before updating it again,
  // thanks to bMustRefreshReplicator flag

  DWORD         dwCurSel  = GetCurSel(lpDomData);
  LPTREERECORD  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                             LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  MfcClearReplicRightPane(hWnd, lpRecord);

  lpDomData->bMustRequestReplicator = TRUE;

  DomGetMenuItemState(hWnd, IDM_REPLIC_INSTALL);
}

//
// Says whether the current selection in the dom referenced by lpDomData
// has a true parent which is a database.
// Second parameter must be a buffer with minimum size MAXOBJECTNAME, or
// a NULL pointer.
// If the function returns true, the buffer will be filled with the
// database name unless the pointer is null
// Returns also TRUE for a database.
// Returns also TRUE for a static direct-sub item of a database
//
BOOL HasTrueParentDB(LPDOMDATA lpDomData, char *dbName)
{
  DWORD         dwCurSel;
  int           CurItemObjType;
  LPTREERECORD  lpRecord;

  dwCurSel = GetCurSel(lpDomData);
  CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  switch (CurItemObjType) {
    case OT_DATABASE:
    case OTR_CDB:
      if (IsNoItem(CurItemObjType, lpRecord->objName))
        return FALSE;
      if (dbName)
        x_strcpy(dbName, lpRecord->objName);
      return TRUE;

    // level 1, child of database
    case OT_TABLE:
    case OT_STATIC_TABLE:
    case OT_VIEW:
    case OT_STATIC_VIEW:
    case OT_PROCEDURE:
    case OT_STATIC_PROCEDURE:
    case OT_SEQUENCE:
    case OT_STATIC_SEQUENCE:
    case OT_STATIC_SCHEMA:
    //case OT_SCHEMAUSER:
    //case OT_STATIC_SCHEMAUSER:
    case OT_SYNONYM:
    case OT_STATIC_SYNONYM:
    case OT_STATIC_DBGRANTEES:
    //case OT_DBGRANT_ACCESY_USER:
    //case OT_STATIC_DBGRANT_ACCESY_USER:
    //case OT_DBGRANT_ACCESN_USER:
    //case OT_STATIC_DBGRANT_ACCESN_USER:
    //case OT_DBGRANT_CREPRY_USER:
    //case OT_STATIC_DBGRANT_CREPRY_USER:
    //case OT_DBGRANT_CREPRN_USER:
    //case OT_STATIC_DBGRANT_CREPRN_USER:
    //case OT_DBGRANT_CRETBY_USER:
    //case OT_STATIC_DBGRANT_CRETBY_USER:
    //case OT_DBGRANT_CRETBN_USER:
    //case OT_STATIC_DBGRANT_CRETBN_USER:
    //case OT_DBGRANT_DBADMY_USER:
    //case OT_STATIC_DBGRANT_DBADMY_USER:
    //case OT_DBGRANT_DBADMN_USER:
    //case OT_STATIC_DBGRANT_DBADMN_USER:
    //case OT_DBGRANT_LKMODY_USER:
    //case OT_STATIC_DBGRANT_LKMODY_USER:
    //case OT_DBGRANT_LKMODN_USER:
    //case OT_STATIC_DBGRANT_LKMODN_USER:
    //case OT_DBGRANT_QRYIOY_USER:
    //case OT_STATIC_DBGRANT_QRYIOY_USER:
    //case OT_DBGRANT_QRYION_USER:
    //case OT_STATIC_DBGRANT_QRYION_USER:
    //case OT_DBGRANT_QRYRWY_USER:
    //case OT_STATIC_DBGRANT_QRYRWY_USER:
    //case OT_DBGRANT_QRYRWN_USER:
    //case OT_STATIC_DBGRANT_QRYRWN_USER:
    //case OT_DBGRANT_UPDSCY_USER:
    //case OT_STATIC_DBGRANT_UPDSCY_USER:
    //case OT_DBGRANT_UPDSCN_USER:
    //case OT_STATIC_DBGRANT_UPDSCN_USER:
    case OT_DBEVENT:
    case OT_STATIC_DBEVENT:
    case OT_STATIC_DBALARM                :   // alarms on database
    case OT_STATIC_DBALARM_CONN_SUCCESS   :
    case OT_STATIC_DBALARM_CONN_FAILURE   :
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
    case OT_S_ALARM_CO_SUCCESS_USER       :
    case OT_S_ALARM_CO_FAILURE_USER       :
    case OT_S_ALARM_DI_SUCCESS_USER       :
    case OT_S_ALARM_DI_FAILURE_USER       :

    // level 2, child of "table of database"
    case OT_INTEGRITY:
    case OT_STATIC_INTEGRITY:
    case OT_RULE:
    case OT_STATIC_RULE:
    case OT_INDEX:
    case OT_STATIC_INDEX:
    case OT_TABLELOCATION:
    case OT_STATIC_TABLELOCATION:
    case OT_S_ALARM_SELSUCCESS_USER:        // alarms on table
    case OT_STATIC_SEC_SEL_SUCC:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_STATIC_SEC_SEL_FAIL:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_STATIC_SEC_DEL_SUCC:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_STATIC_SEC_DEL_FAIL:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_STATIC_SEC_INS_SUCC:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_STATIC_SEC_INS_FAIL:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_STATIC_SEC_UPD_SUCC:
    case OT_S_ALARM_UPDFAILURE_USER:
    case OT_STATIC_SEC_UPD_FAIL:

    //case OT_TABLEGRANT_SEL_USER:
    //case OT_STATIC_TABLEGRANT_SEL_USER:
    //case OT_TABLEGRANT_INS_USER:
    //case OT_STATIC_TABLEGRANT_INS_USER:
    //case OT_TABLEGRANT_UPD_USER:
    //case OT_STATIC_TABLEGRANT_UPD_USER:
    //case OT_TABLEGRANT_DEL_USER:
    //case OT_STATIC_TABLEGRANT_DEL_USER:
    //case OT_TABLEGRANT_REF_USER:
    //case OT_STATIC_TABLEGRANT_REF_USER:
    //case OT_TABLEGRANT_ALL_USER:
    //case OT_STATIC_TABLEGRANT_ALL_USER:

    //case OTR_TABLESYNONYM:
    //case OTRSTATIC__TABLESYNONYM:
    //case OTR_TABLEVIEW:
    //case OTRSTATIC__TABLEVIEW:

    //case OTR_REPLIC_TABLE_CDDS:
    //case OTRSTATIC__REPLIC_TABLE_CDDS:

    // level 2, child of "view of database"
    case OT_VIEWTABLE:
    case OT_STATIC_VIEWTABLE:

    //case OT_VIEWGRANT_SEL_USER:
    //case OT_STATIC_VIEWGRANT_SEL_USER:
    //case OT_VIEWGRANT_INS_USER:
    //case OT_STATIC_VIEWGRANT_INS_USER:
    //case OT_VIEWGRANT_UPD_USER:
    //case OT_STATIC_VIEWGRANT_UPD_USER:
    //case OT_VIEWGRANT_DEL_USER:
    //case OT_STATIC_VIEWGRANT_DEL_USER:

    //case OTR_VIEWSYNONYM:
    //case OTRSTATIC__VIEWSYNONYM:

    // level 2, child of "procedure of database"
    case OT_PROCGRANT_EXEC_USER:
    case OT_STATIC_PROCGRANT_EXEC_USER:
    //case OTR_PROC_RULE:
    //case OTRSTATIC__PROC_RULE:

    // level 2, child of "Sequence of database"
    case OT_SEQUGRANT_NEXT_USER:
    case OT_STATIC_SEQGRANT_NEXT_USER:
    // level 2, child of "dbevent of database"
    //case OT_DBEGRANT_RAISE_USER:
    //case OT_STATIC_DBEGRANT_RAISE_USER:
    //case OT_DBEGRANT_REGTR_USER:
    //case OT_STATIC_DBEGRANT_REGTR_USER:

    // level 2, child of "cdds of database"
    case OT_REPLIC_CDDS_DETAIL:
    case OT_STATIC_REPLIC_CDDS_DETAIL:
    //case OTR_REPLIC_CDDS_TABLE:
    //case OTRSTATIC__REPLIC_CDDS_TABLE:

    // level 2, child of "replicator on database"
    case OT_REPLIC_CONNECTION:
    case OT_STATIC_REPLIC_CONNECTION:
    case OT_REPLIC_CDDS:
    case OT_STATIC_REPLIC_CDDS:
    case OT_REPLIC_MAILUSER:
    case OT_STATIC_REPLIC_MAILUSER:
    case OT_REPLIC_REGTABLE:
    case OT_STATIC_REPLIC_REGTABLE:

    case OT_SCHEMAUSER_TABLE:
    case OT_STATIC_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_STATIC_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:
    case OT_STATIC_SCHEMAUSER_PROCEDURE:

    // level 3, child of "index on table of database"
    //case OTR_INDEXSYNONYM:
    //case OTRSTATIC__INDEXSYNONYM:

    // level 3, child of "rule on table of database"
    case OT_RULEPROC:
    case OT_STATIC_RULEPROC:

      // pre-test for installation level branches
      if (lpRecord->extra[0] == '\0')
        return FALSE;

      if (dbName)
        x_strcpy(dbName, lpRecord->extra);    // parent of level 0
      return TRUE;

    // new style alarms (with 2 sub-branches alarmee and launched dbevent)
    case OT_ALARM_SUBSTATICS:
    case OT_STATIC_ALARMEE:
    case OT_STATIC_ALARM_EVENT:
    case OT_S_ALARM_EVENT:
      if (dbName)
        x_strcpy(dbName, lpRecord->extra);    // parent of level 0
      return TRUE;

    default:
      return FALSE;
  }
  return FALSE;
}

//
//  Says whether the item is a "No xxx" - used for cut/copy menuitems
//
static BOOL NEAR IsNoItem(int objType, char *objName)
{
  if (objName[0]=='\0')
    return TRUE;
  else
    return FALSE;
}

//
//  Enables/disables the menu item, and grays/ungrays the toolbar
//  related button accordingly.
//  (both document toolbar and global toolbar are maintained)
//
//  mode is assumed to be MF_ENABLED or MF_GRAYED exclusively
//
static VOID NEAR EnableItemAndBtn(HMENU hMenu, LPDOMDATA lpDomData, UINT idMenu, UINT mode)
{
  EnableMenuItem(hMenu, idMenu, MF_BYCOMMAND | mode);

  // No action on toolbar for mfc version!
  return;

}


// from main.c
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);

BOOL DomGetMenuItemState(HWND hWnd, UINT itemId)
{
  HWND      hwndMdi;
  HMENU     hMenu;
  LPDOMDATA lpDomData;
  UINT      state;

  hwndMdi = GetVdbaDocumentHandle(hWnd);
  lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  if (!lpDomData)
    return TRUE;

  hMenu = hGwNoneMenu;
  DomUpdateMenu(hwndMdi, lpDomData, TRUE);

  state = GetMenuState(hMenu, itemId, MF_BYCOMMAND);
  if (state == 0xFFFFFFFF)
    return FALSE;   // unknown item
  if (state & MF_GRAYED)
    return FALSE;
  else
    return TRUE;
}

// defaults to FALSE
BOOL CanRegisterAsLink( int hnodestruct, LPTREERECORD lpRecord)
{
  int iobjecttype;

  // manage null case in scratchpad
  if (!lpRecord)
    return FALSE;

  iobjecttype = lpRecord->recType;
  switch (iobjecttype) {
    case OT_STATIC_TABLE:
    case OT_TABLE:
    case OT_STATIC_VIEW:
    case OT_VIEW:
    case OT_STATIC_PROCEDURE:
    case OT_PROCEDURE:
      if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
        return TRUE;
      break;
  }
  return FALSE;
}

// defaults to FALSE
BOOL CanRefreshLink( int hnodestruct, LPTREERECORD lpRecord)
{
  int iobjecttype;

  // manage null case in scratchpad
  if (!lpRecord)
    return FALSE;

  iobjecttype = lpRecord->recType;
  switch (iobjecttype) {
    case OT_TABLE:
      // table: on star database, whatever native or registered as link
      if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
        return TRUE;
      break;

    case OT_VIEW:
      // view : on star database, only if registered as link, not if native
      if (lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
        int objType = getint(lpRecord->szComplim + 4 + STEPSMALLOBJ);
          if (objType == OBJTYPE_STARLINK)  // as opposed to OBJTYPE_STARNATIVE
            return TRUE;
      }
      break;
  }
  return FALSE;
}


int GetUnsolvedItemObjType(LPDOMDATA lpDomData, DWORD dwItem)
{
  LPTREERECORD  lpRecord;

  if (dwItem) {
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (lpRecord)
      if (lpRecord->unsolvedRecType == -1)
        return lpRecord->recType;
      else
        return lpRecord->unsolvedRecType;
  }
  return -1;  // Error
}

//
// VOID DomUpdateMenu(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bActivate);
//
//  Updates the menu when the MDI document is activated or desactivated
//  or when the current selection in the dom changes
//  or when a user action needs the menu to be updated (clipboard, ...)
//
// At this time, hwndMdi not used
//
VOID DomUpdateMenu(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bActivate)
{
  HMENU         hMenu;
  BOOL          bCurSelStatic;
  DWORD         dwCurSel;
  int           CurItemObjType;
  LPTREERECORD  lpRecord;
  DWORD         dwFirstChild;
  int           potAddType = 0;     // Potential add type

  // dummy instructions to skip compiler warnings at level four
  hwndMdi;

  // Don't update the buttons if we are still in create phase
  if (lpDomData->bCreatePhase)
    return;

  hMenu = hGwNoneMenu;

  if (hMenu)
    if (bActivate) {
      bCurSelStatic = IsCurSelObjStatic(lpDomData);
      dwCurSel = GetCurSel(lpDomData);
      CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

      EnableItemAndBtn(hMenu, lpDomData, IDM_PRINT, MF_ENABLED);

    // Star Remove item management
    EnableItemAndBtn(hMenu, lpDomData, IDM_REMOVEOBJECT , MF_GRAYED);
    if (!bCurSelStatic) {
      if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
        switch (CurItemObjType) {
          case OT_TABLE:
            if (getint(lpRecord->szComplim + STEPSMALLOBJ) == OBJTYPE_STARLINK)
              EnableItemAndBtn(hMenu, lpDomData, IDM_REMOVEOBJECT , MF_ENABLED);
            break;
          case OT_VIEW:
            if (getint(lpRecord->szComplim + 4 + STEPSMALLOBJ) == OBJTYPE_STARLINK)
              EnableItemAndBtn(hMenu, lpDomData, IDM_REMOVEOBJECT , MF_ENABLED);
            break;
          case OT_PROCEDURE:
            EnableItemAndBtn(hMenu, lpDomData, IDM_REMOVEOBJECT , MF_ENABLED);  // Always of type link
            break;
        }
      }
    }

      // test existence of modeless dialog - hDlgSearch in main.c
      if (hDlgSearch)
        EnableItemAndBtn(hMenu, lpDomData, IDM_FIND,            MF_GRAYED);
      else
        EnableItemAndBtn(hMenu, lpDomData, IDM_FIND,            MF_ENABLED);

      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_EXPANDONE,   MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_EXPANDBRANCH,MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_EXPANDALL   ,MF_ENABLED);

      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_COLLAPSEONE   , MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_COLLAPSEBRANCH, MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_CLASSB_COLLAPSEALL   , MF_ENABLED);

      EnableItemAndBtn(hMenu, lpDomData, IDM_SHOW_SYSTEM  , MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_FILTER       , MF_ENABLED);

      // Properties
      if (bCurSelStatic)
        EnableItemAndBtn(hMenu, lpDomData, IDM_PROPERTIES, MF_GRAYED);
      else {
        if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName))
          EnableItemAndBtn(hMenu, lpDomData, IDM_PROPERTIES, MF_GRAYED);
        else
          if (HasProperties4display(CurItemObjType))
            EnableItemAndBtn(hMenu, lpDomData, IDM_PROPERTIES, MF_ENABLED);
          else
            EnableItemAndBtn(hMenu, lpDomData, IDM_PROPERTIES, MF_GRAYED);
      }
      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI)
        EnableItemAndBtn(hMenu, lpDomData, IDM_PROPERTIES, MF_GRAYED);



//JFS Begin these menus are enabled only if replication objects are installed

      // Replicator : Only enabled if we are on the "replicator" item
      // and replicator is not installed yet
      EnableMenuItem(hMenu, IDM_REPLIC_INSTALL , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_PROPAG  , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_BUILDSRV, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_RECONCIL, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_DEREPLIC, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_MOBILE,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_ARCCLEAN, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_REPMOD,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_CREATEKEYS, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_ACTIVATE,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_DEACTIVATE, MF_BYCOMMAND | MF_GRAYED);

      if (CurItemObjType == OT_STATIC_REPLICATOR) {
        if (lpDomData->bMustRequestReplicator) {
          lpDomData->lastDwCurSel = dwCurSel;
          lpDomData->bFirstTimeOnThisCurSel = TRUE;
          lpDomData->bMustRequestReplicator = FALSE;
        }
        else {
          if (dwCurSel != lpDomData->lastDwCurSel) {
            lpDomData->lastDwCurSel = dwCurSel;
            lpDomData->bFirstTimeOnThisCurSel = TRUE;
          }
          else
            lpDomData->bFirstTimeOnThisCurSel = FALSE;
        }
        if (lpDomData->bFirstTimeOnThisCurSel) {
          ShowHourGlass();
          lpDomData->iReplicVersion = GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                                                  lpRecord->extra, lpRecord->ownerName);
          RemoveHourGlass();
        }
        if (lpDomData->iReplicVersion==REPLIC_NOINSTALL)
           EnableMenuItem(hMenu, IDM_REPLIC_INSTALL , MF_BYCOMMAND | MF_ENABLED);
        else {
           if (lpDomData->bFirstTimeOnThisCurSel) {       // 26/2/97 : added cache for ReplicatorLocalDbInstalled() calls optimization
              lpDomData->LocalDbInstalledStatus = ReplicatorLocalDbInstalled(lpDomData->psDomNode->nodeHandle,
                                                                             lpRecord->extra,lpDomData->iReplicVersion);
           }
           if (lpDomData->LocalDbInstalledStatus == RES_ERR)
               EnableMenuItem(hMenu, IDM_REPLIC_DEREPLIC, MF_BYCOMMAND | MF_ENABLED);
           else  {
               if (lpDomData->LocalDbInstalledStatus == RES_SUCCESS) {
                  EnableMenuItem(hMenu, IDM_REPLIC_PROPAG  , MF_BYCOMMAND | MF_ENABLED);
                  if (lpDomData->ingresVer < OIVERS_25)
                    EnableMenuItem(hMenu, IDM_REPLIC_BUILDSRV, MF_BYCOMMAND | MF_ENABLED);
                  EnableMenuItem(hMenu, IDM_REPLIC_RECONCIL, MF_BYCOMMAND | MF_ENABLED);
                  EnableMenuItem(hMenu, IDM_REPLIC_DEREPLIC, MF_BYCOMMAND | MF_ENABLED);
                  EnableMenuItem(hMenu, IDM_REPLIC_ARCCLEAN, MF_BYCOMMAND | MF_ENABLED);
                  EnableMenuItem(hMenu, IDM_REPLIC_REPMOD,   MF_BYCOMMAND | MF_ENABLED);
                  if (lpDomData->iReplicVersion==REPLIC_V105) 
                    EnableMenuItem(hMenu, IDM_REPLIC_MOBILE,   MF_BYCOMMAND | MF_ENABLED);
               }
           }
        }

//JFS End
      }

      EnableItemAndBtn(hMenu, lpDomData, IDM_NEWWINDOW     , MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_TEAROUT       , MF_ENABLED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_RESTARTFROMPOS, MF_ENABLED);
      //EnableItemAndBtn(hMenu, lpDomData, IDM_SCRATCHPAD    , MF_ENABLED);

      EnableItemAndBtn(hMenu, lpDomData, IDM_LOCATE        , MF_ENABLED);

      // if focus gained, enabled or disabled according to current
      // dom item type and complimentary info

      // Add object : special management
      if (dwCurSel==0 || CurItemObjType == OT_STATIC_R_GRANT
                      || CurItemObjType == OT_STATIC_SYNONYMED
                      || CurItemObjType == OT_STATIC_GROUPUSER
                      || CurItemObjType == OT_GROUPUSER
                      || CurItemObjType == OT_STATIC_R_USERGROUP
                      || CurItemObjType == OT_STATIC_R_REPLIC_TABLE_CDDS
                      || CurItemObjType == OT_STATIC_REPLICATOR
                      || CurItemObjType == OT_STATIC_ICE_SECURITY
                      || CurItemObjType == OT_STATIC_ICE_BUNIT_SECURITY
                      || CurItemObjType == OT_STATIC_ICE_SERVER)
        EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);
      else {
        if (bCurSelStatic) {
          dwFirstChild = GetFirstImmediateChild(lpDomData, dwCurSel, 0);
          if (IsItemObjStatic(lpDomData, dwFirstChild))
            potAddType = GetChildType(GetItemObjType(lpDomData, dwFirstChild), FALSE);
          else
            potAddType = GetChildType(CurItemObjType, FALSE);
          // special for static having sub statics not expanded yet
          switch (CurItemObjType) {
            case OT_STATIC_DBGRANTEES:
            case OT_STATIC_INSTALL_GRANTEES:
              if (potAddType == OT_STATIC_DUMMY)
                potAddType = OT_DBGRANT_ACCESY_USER;
              break;
            case OT_STATIC_INSTALL_ALARMS:
              if (potAddType == OT_STATIC_DUMMY)
                potAddType = OT_S_ALARM_CO_SUCCESS_USER;
              break;
          }
        }
        else
          potAddType = CurItemObjType;
        if (potAddType != OT_STATIC_DUMMY && CanObjectBeAdded(potAddType))
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_ENABLED);
        else
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);
      }
      // Emb 15/11/95 - post-manage gray for non-static at root level
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwCurSel)==0
          && !bCurSelStatic)
        EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);


      // Alter - Drop object
      if (bCurSelStatic) {
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT , MF_GRAYED);
      }
      else {
        if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName))
          EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT , MF_GRAYED);
        else
          if (CanObjectBeDeleted(CurItemObjType, lpRecord->objName,
                                                lpRecord->ownerName))
            EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT , MF_ENABLED);
          else
            EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT , MF_GRAYED);

        if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName))
          EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
        else {
          // special management due to load feature, added 21/11/96
          int oldOIVers = GetOIVers();
          SetOIVers(lpDomData->ingresVer);
          if (CanObjectBeAltered(CurItemObjType, lpRecord->objName, lpRecord->ownerName)
		&& (!IsVW() 
		|| (IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0))
		)
            EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT , MF_ENABLED);
          else
            EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT , MF_GRAYED);
          SetOIVers(oldOIVers);
        }
      }
      // Emb 15/11/95 - post-manage gray for non-static at root level
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwCurSel)==0
          && !bCurSelStatic) {
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
      }
 
      // Restricted features if gateway
      //if (lpDomData->ingresVer==OIVERS_NOTOI) {
        //EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
        //if (potAddType != OT_VIEW)
        //  EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
      //}

      // Refresh button : always enabled, first radio will be disabled
      // if related type or if expansion would only create sub-statics
      EnableItemAndBtn(hMenu, lpDomData, IDM_REFRESH , MF_ENABLED);

      // Cut to clipboard - TO BE FINISHED
      EnableItemAndBtn(hMenu, lpDomData, IDM_CUT  , MF_GRAYED);

      // Copy to clipboard
      if (bCurSelStatic)
        EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_GRAYED);
      else {
        if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName))
          EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_GRAYED);
        else
          if (CanObjectBeCopied(CurItemObjType, lpRecord->objName, lpRecord->ownerName))
            EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_ENABLED);
          else
            EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_GRAYED);
      }

      // Restricted features if gateway
      //if (lpDomData->ingresVer==OIVERS_NOTOI)
        //EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_GRAYED);

      // Paste object from clipboard
      if (DomClipboardObjType) {
        if (bCurSelStatic) {
          dwFirstChild = GetFirstImmediateChild(lpDomData, dwCurSel, 0);
          if (IsItemObjStatic(lpDomData, dwFirstChild))
            potAddType = GetChildType(GetItemObjType(lpDomData, dwFirstChild), FALSE);
          else
            potAddType = GetChildType(CurItemObjType, FALSE);
        }
        else
          if (lpRecord)
            potAddType = lpRecord->recType;
          else
            potAddType = OT_ERROR;
        if (IsRelatedType(potAddType)
            || !CanObjectBeAdded(potAddType)
            || (GetBasicType(potAddType) != DomClipboardObjType) )
          EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);
        else
          EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_ENABLED);
        // Post-manage: no paste on openingres of a desktop table
      }
      else
        EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);

      // Restricted features if gateway
      //if (lpDomData->ingresVer==OIVERS_NOTOI)
       // EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);
      if (lpDomData->bIsVectorWise && (CurItemObjType == OT_TABLE ||
		CurItemObjType == OT_VIEW ||
		CurItemObjType == OT_SYNONYM ||
		CurItemObjType == OT_SCHEMAUSER_TABLE ||
		CurItemObjType == OT_SCHEMAUSER_VIEW))
      {
		EnableItemAndBtn(hMenu, lpDomData, IDM_ALTERDB, MF_GRAYED);
        	EnableItemAndBtn(hMenu, lpDomData, IDM_SYSMOD,  MF_GRAYED);

        	EnableItemAndBtn(hMenu, lpDomData, IDM_AUDIT   , MF_GRAYED);
		EnableItemAndBtn(hMenu, lpDomData, IDM_VERIFYDB ,  MF_GRAYED);

		EnableItemAndBtn(hMenu, lpDomData, IDM_GENSTAT , MF_ENABLED);
		EnableItemAndBtn(hMenu, lpDomData, IDM_DISPSTAT ,  MF_ENABLED);

		if (getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0)
		{
			EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT , MF_ENABLED);
			EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD, MF_ENABLED);
		}
		else
		{
			EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT ,MF_GRAYED);
			EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD,MF_GRAYED);
		}

		EnableItemAndBtn(hMenu, lpDomData, IDM_UNLOADDB,  MF_ENABLED);
        	EnableItemAndBtn(hMenu, lpDomData, IDM_COPYDB,    MF_ENABLED);
      }
      // audit, genstat... : only on database direct child items
      else if (HasTrueParentDB(lpDomData, NULL)) {
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTERDB, MF_ENABLED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_SYSMOD,  MF_ENABLED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_AUDIT   , MF_ENABLED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_GENSTAT , MF_ENABLED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_DISPSTAT ,  MF_ENABLED);
	if (lpDomData->bIsVectorWise && (CurItemObjType == OT_DATABASE 
		|| CurItemObjType == OT_STATIC_TABLE))
	{
		EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT ,MF_GRAYED);
		EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD,MF_GRAYED);
	}
	else
	{
		EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT ,MF_ENABLED);
		EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD,MF_ENABLED);
	}
        EnableItemAndBtn(hMenu, lpDomData, IDM_VERIFYDB ,  MF_ENABLED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_UNLOADDB,  MF_ENABLED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_COPYDB,    MF_ENABLED);
      }
      else {
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTERDB, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_SYSMOD,  MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_AUDIT   , MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_GENSTAT , MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_DISPSTAT ,  MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT ,MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD,MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_VERIFYDB ,  MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_UNLOADDB,  MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_COPYDB,    MF_GRAYED);
      }
      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI) {
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTERDB, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_SYSMOD,  MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_AUDIT   , MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_GENSTAT , MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_DISPSTAT ,  MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_CHECKPOINT ,MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_ROLLFORWARD,MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_VERIFYDB ,  MF_GRAYED);

        EnableItemAndBtn(hMenu, lpDomData, IDM_UNLOADDB,  MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_COPYDB,    MF_GRAYED);
      }



      // Populate : only if item is a table or Database
      switch (CurItemObjType) {
        case OT_DATABASE:
        case OT_TABLE:
          if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName)) 
          {
            EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE, MF_GRAYED);
            EnableItemAndBtn(hMenu, lpDomData, IDM_EXPORT, MF_GRAYED);
          }
          else 
          {
            EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE, MF_ENABLED);
            EnableItemAndBtn(hMenu, lpDomData, IDM_EXPORT, MF_ENABLED);
          }
          break;
        default:
          EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_EXPORT, MF_GRAYED);
          break;
      }

      // Restricted features if gateway
      //if (lpDomData->ingresVer==OIVERS_NOTOI)
      //  EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE, MF_GRAYED);

      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI) {
        if (CurItemObjType == OT_STATIC_DATABASE || CurItemObjType == OT_DATABASE) {
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT,  MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE,   MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_EXPORT,   MF_GRAYED);
        }
      }

      // Grant - Revoke
      switch (CurItemObjType) {
        case OT_DATABASE:
        case OT_TABLE:
        case OT_VIEW:
        case OT_PROCEDURE:
        case OT_SEQUENCE:
        case OT_SCHEMAUSER_TABLE:
        case OT_SCHEMAUSER_VIEW:
        case OT_SCHEMAUSER_PROCEDURE:
        case OT_DBEVENT:
        case OT_ROLE:
          if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName)) {
            EnableMenuItem(hMenu,  IDM_GRANT,       MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu,  IDM_REVOKE,      MF_BYCOMMAND | MF_GRAYED);
          }
          else {
            EnableMenuItem(hMenu,  IDM_GRANT,       MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu,  IDM_REVOKE,      MF_BYCOMMAND | MF_ENABLED);
          }
          break;

        default:
          EnableMenuItem(hMenu,  IDM_GRANT,       MF_BYCOMMAND | MF_GRAYED);
          EnableMenuItem(hMenu,  IDM_REVOKE,      MF_BYCOMMAND | MF_GRAYED);
          break;
      }

      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI) {
        EnableMenuItem(hMenu,  IDM_GRANT,       MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu,  IDM_REVOKE,      MF_BYCOMMAND | MF_GRAYED);
      }
	
      //Create index for VW unindexed tables only
      if (lpRecord && IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == IDX_VW)
        EnableMenuItem(hMenu, IDM_CREATEIDX, MF_BYCOMMAND | MF_ENABLED);
      else
        EnableMenuItem(hMenu, IDM_CREATEIDX, MF_BYCOMMAND | MF_GRAYED);

      // Modify structure
      if (lpRecord==0 || IsNoItem(CurItemObjType, lpRecord->objName))
        EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);
      else
        if (CanObjectStructureBeModified(CurItemObjType) && (!lpDomData->bIsVectorWise ||
			(IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0)))
          EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_ENABLED);
        else
          EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);

      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI)
        EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);

      // Restricted features if DDB
      if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
        EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);

      // Not acceptable for system object
      if (lpRecord != 0 && IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName))
        EnableMenuItem(hMenu, IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);

      // ingres desktop specific
      EnableMenuItem(hMenu, IDM_LOAD,       MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_UNLOAD,     MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_DOWNLOAD,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_RUNSCRIPTS, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_UPDSTAT,    MF_BYCOMMAND | MF_GRAYED);
      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI)
        ;
      else {
        switch (CurItemObjType) {
          case OT_DATABASE:
          case OT_TABLE:
          case OT_INDEX:
            if (lpRecord != 0
                && !IsNoItem(CurItemObjType, lpRecord->objName)) {
              // only database for load/unload
              if (CurItemObjType == OT_DATABASE) {
                EnableMenuItem(hMenu, IDM_LOAD,       MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenu, IDM_UNLOAD,     MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenu, IDM_DOWNLOAD,   MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenu, IDM_RUNSCRIPTS, MF_BYCOMMAND | MF_ENABLED);
              }
              // database, table or index for update statistics
              EnableMenuItem(hMenu, IDM_UPDSTAT, MF_BYCOMMAND | MF_ENABLED);
            }
        }
      }

      // Spacecalc - 14/11/96
      EnableItemAndBtn(hMenu, lpDomData, IDM_SPACECALC, MF_ENABLED);

      // Restricted features if gateway
      if (lpDomData->ingresVer==OIVERS_NOTOI)
        EnableItemAndBtn(hMenu, lpDomData, IDM_SPACECALC, MF_GRAYED);

      // Star specific menuitems management
      if (CanRegisterAsLink(lpDomData->psDomNode->nodeHandle, lpRecord))
        EnableItemAndBtn(hMenu, lpDomData, IDM_REGISTERASLINK, MF_ENABLED);
      else
        EnableItemAndBtn(hMenu, lpDomData, IDM_REGISTERASLINK, MF_GRAYED);
      if (CanRefreshLink(lpDomData->psDomNode->nodeHandle, lpRecord))
        EnableItemAndBtn(hMenu, lpDomData, IDM_REFRESHLINK, MF_ENABLED);
      else
        EnableItemAndBtn(hMenu, lpDomData, IDM_REFRESHLINK, MF_GRAYED);

      // STAR post-corrections for previous items
      // 1/2: restrictions for DDB
      if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
        int iobjecttype = lpRecord->recType;

        // No cut, copy, paste whatever item is
        EnableItemAndBtn(hMenu, lpDomData, IDM_CUT, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_COPY, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);

        // no alter, grant, revoke, populate whatever item is
        EnableItemAndBtn(hMenu, lpDomData, IDM_GRANT, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_REVOKE, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_POPULATE, MF_GRAYED);
        EnableItemAndBtn(hMenu, lpDomData, IDM_EXPORT, MF_GRAYED);

        // procedure: no create
        if (iobjecttype == OT_PROCEDURE || iobjecttype == OT_STATIC_PROCEDURE)
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);

        // Sequence: no create
        if (iobjecttype == OT_SEQUENCE || iobjecttype == OT_STATIC_SEQUENCE)
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);

        // index: no create, alter, remove
        if (iobjecttype == OT_INDEX || iobjecttype == OT_STATIC_INDEX) {
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
        }
        // table, view, procedure: drop according to native/linked type
        switch (iobjecttype) {
          case OT_TABLE:
            if (getint(lpRecord->szComplim + STEPSMALLOBJ) != OBJTYPE_STARNATIVE)
              EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
            break;
          case OT_VIEW:
            if (getint(lpRecord->szComplim + 4 + STEPSMALLOBJ) != OBJTYPE_STARNATIVE)
              EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
            break;
          case OT_PROCEDURE:
            EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);  // since always of type link
            break;
        }
      }
      // 2/2: restrictions for CDB
      if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_COORDINATOR) {
        int iobjecttype = lpRecord->recType;

        // No Add/Alter/Drop/Paste for tables, views and procedures
        if (iobjecttype == OT_TABLE || iobjecttype == OT_STATIC_TABLE || 
            iobjecttype == OT_VIEW  || iobjecttype == OT_STATIC_VIEW  ||
            iobjecttype == OT_PROCEDURE  || iobjecttype == OT_STATIC_PROCEDURE ||
            iobjecttype == OT_SEQUENCE   || iobjecttype == OT_STATIC_SEQUENCE) {
          EnableItemAndBtn(hMenu, lpDomData, IDM_ADDOBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_ALTEROBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_DROPOBJECT, MF_GRAYED);
          EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);
        }
      }

      // New IDM_INFODB menuitem management
      EnableMenuItem(hMenu, IDM_INFODB,   MF_BYCOMMAND | MF_GRAYED);
      if (lpDomData->ingresVer > OIVERS_20) {
          if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_REGULAR) {
            int iobjecttype = lpRecord->recType;
            if (iobjecttype == OT_DATABASE)
              EnableMenuItem(hMenu, IDM_INFODB,   MF_BYCOMMAND | MF_ENABLED);
          }
      }
      // New IDM_DUPLICATEDB menuitem management
      EnableMenuItem(hMenu, IDM_DUPLICATEDB,   MF_BYCOMMAND | MF_GRAYED);
      if (lpDomData->ingresVer > OIVERS_20) {
        if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_REGULAR) {
          if (lpRecord->recType == OT_DATABASE && !IsVW())
            EnableMenuItem(hMenu, IDM_DUPLICATEDB,   MF_BYCOMMAND | MF_ENABLED);
        }
      }

      // New IDM_SECURITYAUDIT menuitem management
      EnableMenuItem(hMenu, IDM_SECURITYAUDIT,   MF_BYCOMMAND | MF_GRAYED);
      if (lpRecord != 0 && lpRecord->recType == OT_STATIC_INSTALL_SECURITY)
        if (!IsNoItem(lpRecord->recType, lpRecord->objName))
          EnableMenuItem(hMenu, IDM_SECURITYAUDIT,   MF_BYCOMMAND | MF_ENABLED);

      // New IDM_EXPIREDATE and FASTLOAD menuitems management : only on regular databases on pure ingres nodes
      // same for IDM_JOURNALING
      EnableMenuItem(hMenu, IDM_EXPIREDATE, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_FASTLOAD,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_JOURNALING, MF_BYCOMMAND | MF_GRAYED);
      if (lpDomData->ingresVer!=OIVERS_NOTOI) {
        if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_REGULAR)
          if (CurItemObjType == OT_TABLE && !IsNoItem(CurItemObjType, lpRecord->objName))
            if (!IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName)) {
              EnableMenuItem(hMenu, IDM_EXPIREDATE, MF_BYCOMMAND | MF_ENABLED);
			  if (!IsVW() || (IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0)) {
                if (lpDomData->ingresVer > OIVERS_20) 
                  EnableMenuItem(hMenu, IDM_FASTLOAD,   MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenu, IDM_JOURNALING, MF_BYCOMMAND | MF_ENABLED);
			  }
            }
      }
      // Manage IDM_COMMENT
      EnableMenuItem(hMenu, IDM_COMMENT, MF_BYCOMMAND | MF_GRAYED);
      if (lpDomData->ingresVer!=OIVERS_NOTOI) {
        if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_REGULAR)
          if ((CurItemObjType == OT_TABLE || CurItemObjType == OT_VIEW) && !IsNoItem(CurItemObjType, lpRecord->objName))
            if (!IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName)) {
              EnableMenuItem(hMenu, IDM_COMMENT, MF_BYCOMMAND | MF_ENABLED);
            }
      }
      // Manage IDM_USERMOD
      EnableMenuItem(hMenu, IDM_USERMOD, MF_BYCOMMAND | MF_GRAYED);
      if (lpDomData->ingresVer>=OIVERS_26 && HasTrueParentDB(lpDomData, NULL)) {
        if (lpRecord != 0 && 
			(!IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName)|| lpRecord->parentDbType == DBTYPE_COORDINATOR)
		    ) {
          if (lpRecord->parentDbType != DBTYPE_DISTRIBUTED && (!IsVW() || (IsVW() && CurItemObjType == OT_DATABASE)))
            EnableMenuItem(hMenu, IDM_USERMOD, MF_BYCOMMAND | MF_ENABLED);
        }
      }
      // Replicator on Ingres 2.5 specific
      if (lpRecord && lpDomData->ingresVer >= OIVERS_25) {
        if (!IsNoItem(lpRecord->recType, lpRecord->objName)) {
          switch (lpRecord->recType) {
            case OTR_REPLIC_CDDS_TABLE:
            case OT_REPLIC_REGTABLE:
              EnableMenuItem(hMenu, IDM_REPLIC_CREATEKEYS, MF_BYCOMMAND | MF_ENABLED);
              break;
          }
          switch (lpRecord->recType) {
            case OT_REPLIC_CDDS:
            case OTR_REPLIC_TABLE_CDDS:
            case OTR_REPLIC_CDDS_TABLE:
            case OT_REPLIC_REGTABLE:
              EnableMenuItem(hMenu, IDM_REPLIC_ACTIVATE,   MF_BYCOMMAND | MF_ENABLED);
              EnableMenuItem(hMenu, IDM_REPLIC_DEACTIVATE, MF_BYCOMMAND | MF_ENABLED);
              break;
          }
        }
      }

    }
    else {
      // all items are grayed if focus is lost
      // WE DON'T GRAY BUTTON ITEMS EXCEPT THOSE IN THE MAIN BUTTON BAR

      // Star Remove item management
      EnableMenuItem(hMenu, IDM_REMOVEOBJECT , MF_BYCOMMAND | MF_GRAYED);

      EnableItemAndBtn(hMenu, lpDomData, IDM_FIND,            MF_GRAYED);

      EnableMenuItem(hMenu, IDM_CLASSB_EXPANDONE     , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_CLASSB_EXPANDBRANCH  , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_CLASSB_EXPANDALL     , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_CLASSB_COLLAPSEONE   , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_CLASSB_COLLAPSEBRANCH, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_CLASSB_COLLAPSEALL   , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_REFRESH    , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_SHOW_SYSTEM, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_FILTER     , MF_BYCOMMAND | MF_GRAYED);

      // Jan 21, 97: update all menus for Show System, resetting the flag
      // so we won't get into trouble after closing the last document
      CheckMenuItem(hGwNoneMenu, IDM_SHOW_SYSTEM   , MF_BYCOMMAND | MF_UNCHECKED);

      EnableMenuItem(hMenu, IDM_PROPERTIES,  MF_BYCOMMAND | MF_GRAYED);

      // Replicator
      EnableMenuItem(hMenu, IDM_REPLIC_INSTALL , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_PROPAG  , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_BUILDSRV, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_RECONCIL, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_DEREPLIC, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_ARCCLEAN, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_REPMOD,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_CREATEKEYS, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_ACTIVATE,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REPLIC_DEACTIVATE, MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_ADDOBJECT     , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_ALTEROBJECT   , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_DROPOBJECT    , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_NEWWINDOW     , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_TEAROUT       , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_RESTARTFROMPOS, MF_BYCOMMAND | MF_GRAYED);
      //EnableMenuItem(hMenu, IDM_SCRATCHPAD    , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu, IDM_LOCATE        , MF_BYCOMMAND | MF_GRAYED);

      // Print (in main bar)
      EnableItemAndBtn(hMenu, lpDomData, IDM_PRINT  , MF_GRAYED);

      // Cut/Copy/Paste
      EnableItemAndBtn(hMenu, lpDomData, IDM_CUT  , MF_GRAYED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_COPY , MF_GRAYED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_PASTE, MF_GRAYED);

      // 'Operations' menu
      EnableItemAndBtn(hMenu, lpDomData, IDM_ALTERDB, MF_GRAYED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_SYSMOD,  MF_GRAYED);
      EnableItemAndBtn(hMenu, lpDomData, IDM_USERMOD, MF_GRAYED);

      // Populate, audit and genstat
      EnableMenuItem(hMenu, IDM_POPULATE, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_EXPORT, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_AUDIT   , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_GENSTAT , MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu,  IDM_DISPSTAT ,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu,  IDM_CHECKPOINT , MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu,  IDM_ROLLFORWARD, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu,  IDM_VERIFYDB ,   MF_BYCOMMAND | MF_GRAYED);

      EnableMenuItem(hMenu,  IDM_UNLOADDB ,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu,  IDM_COPYDB ,     MF_BYCOMMAND | MF_GRAYED);

      // Grant - Revoke
      EnableMenuItem(hMenu,  IDM_GRANT,       MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu,  IDM_REVOKE,      MF_BYCOMMAND | MF_GRAYED);

      // Modify structure
      EnableMenuItem(hMenu,  IDM_MODIFYSTRUCT, MF_BYCOMMAND | MF_GRAYED);

      // Create Index for unindexed VW tables
      EnableMenuItem(hMenu,  IDM_CREATEIDX, MF_BYCOMMAND | MF_GRAYED);
	  
      // ingres desktop specific
      EnableMenuItem(hMenu, IDM_LOAD,       MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_UNLOAD,     MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_DOWNLOAD,   MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_RUNSCRIPTS, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_UPDSTAT,    MF_BYCOMMAND | MF_GRAYED);

	    // Spacecalc - 14/11/96
      EnableItemAndBtn(hMenu, lpDomData, IDM_SPACECALC, MF_GRAYED);

      // Star menuitems management
      EnableMenuItem(hMenu, IDM_REGISTERASLINK, MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_REFRESHLINK, MF_BYCOMMAND | MF_GRAYED);

      // New C++ menuitems management
      EnableMenuItem(hMenu, IDM_FASTLOAD,       MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_INFODB,         MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_DUPLICATEDB,    MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_SECURITYAUDIT,  MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_EXPIREDATE,     MF_BYCOMMAND | MF_ENABLED);
      EnableMenuItem(hMenu, IDM_COMMENT,        MF_BYCOMMAND | MF_GRAYED);
      EnableMenuItem(hMenu, IDM_USERMOD,        MF_BYCOMMAND | MF_GRAYED);
    }
}

//------------------------------------------------------------------------
//
// Print functions
//
// This section is derived from vo sources menu\menutree.c and ide\stddlg.c
//
//  Use functions in print.c, defined in print.e, both sources taken from vo
//
//------------------------------------------------------------------------

#define MAX_STRINGLEN BUFSIZE
static HDC DOM_PrintStart(HWND hWnd, LPSTR lpBuf, BOOL bTrunc);
static void DOM_OutputAll(HDC hDCPrinter,HWND hwnd,HWND hList,LPSTR lpTitle,BOOL bSelOnly, LPDOMDATA lpDomData);
static BOOL DOM_GetPrinterExtents(HDC hDCPrinter,LPPOINT ptPageSize,LPPOINT ptFontSize);
static void DOM_PrintHeader(HDC hDCPrinter,LPSTR lpTitle,POINT ptPageSize,POINT ptFontSize,UINT uiPage);

static LPFONTINFO GetlpFontInfoFromDOM(HWND hWndMdi, LPDOMDATA lpDomData)
{
  static FONTINFO theFontInfo;

  HFONT       hTreeFont;
  LOGFONT     LogFont;
  HFONT       hOldFont;
  HDC         hDC;
  TEXTMETRIC  tm;
  int         iPix;

  // Pick font used in the tree
  hTreeFont = (HFONT)SendMessage(lpDomData->hwndTreeLb, WM_GETFONT, 0, 0L);
  if (!hTreeFont) {
    // TEST : ARIAL 11
    //hTreeFont = (HFONT)GetStockObject(SYSTEM_FONT);
    _fmemset(&LogFont, 0, sizeof(LogFont));
    LogFont.lfHeight        = 18; // 24 - 36
    LogFont.lfWidth         = 0;
    LogFont.lfEscapement    = 0;
    LogFont.lfOrientation   = 0;
    LogFont.lfWeight        = 400;
    LogFont.lfItalic        = 0;
    LogFont.lfUnderline     = 0;
    LogFont.lfStrikeOut     = 0;
    LogFont.lfCharSet       = 0;
    LogFont.lfOutPrecision  = 3;
    LogFont.lfClipPrecision = 2;
    LogFont.lfQuality       = 22;
    lstrcpy(LogFont.lfFaceName, "Arial");
    hTreeFont = CreateFontIndirect(&LogFont);

    // see cavo.ini : ToolsFont=Arial,11,34,0,400,0,0,3,2,1
    // see caci.ini : StandardFont=Arial,10,34,0,400
  }

  // fill the FONTINFO structure starting from the font handle
  // This code taken from vo source cavosed\cavosed.c, function PrintServer
  GetObject(hTreeFont, sizeof(LogFont), &LogFont);

  _fmemset(&theFontInfo, 0, sizeof(theFontInfo));
  lstrcpy(theFontInfo.achFaceName, LogFont.lfFaceName);
  theFontInfo.iWeight   = LogFont.lfWeight;
  theFontInfo.bPitch    = LogFont.lfPitchAndFamily;
  theFontInfo.bCharSet  = LogFont.lfCharSet;
  theFontInfo.hFont     = hTreeFont;
  theFontInfo.bItalics  = LogFont.lfItalic;

  // Jeff's useful stuff
  theFontInfo.lfEscapement    = LogFont.lfEscapement;
  theFontInfo.lfOutPrecision  = LogFont.lfOutPrecision;
  theFontInfo.lfClipPrecision = LogFont.lfClipPrecision;
  theFontInfo.lfQuality       = LogFont.lfQuality;

  hDC = GetDC(hWndMdi);
  hOldFont = SelectObject(hDC, hTreeFont);
  GetTextMetrics(hDC, &tm);
  SelectObject(hDC, hOldFont);

  //  MS0001 - convert pixels to points
  iPix = GetDeviceCaps(hDC, LOGPIXELSY);
  ReleaseDC(hWndMdi, hDC);
  //  MS0003 - took out float cast
  theFontInfo.iPoints = (72*(tm.tmHeight - tm.tmInternalLeading))/ iPix;

  DeleteObject(hTreeFont);    // Free object - TO BE REMOVED IF SYSTEM FONT

  return &theFontInfo;
}

// from ide\stddlg.c
VOID FAR CreateLogFont(LPFONTINFO lpFontInfo, LPLOGFONT lpLogFont, HDC hDCGiven)
{
  HWND    hwnd;
  HDC     hDC;

  if (hDCGiven == (HDC) 0)
  {
    hwnd    = GetActiveWindow();
    hDC     = GetDC(hwnd);
  }
  else
  {
    hDC = hDCGiven;
  }
  memset(lpLogFont, 0, sizeof(LOGFONT));

  lstrcpy(lpLogFont->lfFaceName, FONT_NAME(lpFontInfo));
  lpLogFont->lfHeight = -(FONT_POINTS(lpFontInfo) * GetDeviceCaps(hDC, LOGPIXELSY) / 72);
  lpLogFont->lfPitchAndFamily = FONT_PITCH(lpFontInfo);
  lpLogFont->lfCharSet = FONT_CHARSET(lpFontInfo);
  lpLogFont->lfWeight = FONT_WEIGHT(lpFontInfo);

  lpLogFont->lfQuality = PROOF_QUALITY;
  lpLogFont->lfOutPrecision =OUT_TT_PRECIS;
  lpLogFont->lfItalic = lpFontInfo->bItalics;

  lpLogFont->lfEscapement   = lpFontInfo->lfEscapement   ;
  lpLogFont->lfOutPrecision = lpFontInfo->lfOutPrecision ;
  lpLogFont->lfClipPrecision= lpFontInfo->lfClipPrecision;
  lpLogFont->lfQuality      = lpFontInfo->lfQuality      ;





  if (hDCGiven == (HDC) 0)
  {
    ReleaseDC(hwnd, hDC);
  }

  return;

}   // end of CreateLogFont(LPFONTINFO lpFontInfo, LPLOGFONT lpLogFont)

// from menu\menutree.c (fonction MEditPrint)
//Emb : #define APPEND_BLANK(p)  (StrCatChar (p,0x20))
VOID PrintDom(HWND hwndMdi, LPDOMDATA lpDomData)
{
  HWND  hList;
  HDC   hDCPrinter;
  UCHAR bufRs[BUFSIZE];
  LPSTR lpBuf;

  hList = lpDomData->hwndTreeLb;

  lpBuf = ESL_AllocMem(2*MAX_STRINGLEN);
  if (!lpBuf)
    return;
  *lpBuf=0;

  // Make modeless box title
  LoadString(hResource, IDS_DOMDOC_PRINTING, bufRs, sizeof(bufRs));
  wsprintf(lpBuf, bufRs, GetVirtNodeName(lpDomData->psDomNode->nodeHandle));

  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_PRINTBOX));
  hDCPrinter = DOM_PrintStart(hwndMdi, lpBuf, FALSE);
  lpHelpStack = StackObject_POP (lpHelpStack);
  if (hDCPrinter) {
    SetMapMode(hDCPrinter, MM_TEXT);
    //Emb : APPEND_BLANK(lpBuf);
    //Emb : APPEND_BLANK(lpBuf);
    //Emb : APPEND_BLANK(lpBuf);
    //Emb : lstrcat(lpBuf, _strdate(resStr));
    //Emb : APPEND_BLANK(lpBuf);
    //Emb : APPEND_BLANK(lpBuf);
    //Emb : lstrcat(lpBuf, _strtime(resStr));
    DOM_OutputAll(hDCPrinter, hwndMdi, hList, lpBuf, FALSE, lpDomData);   // Added lpDomData for IPM tree print
    PrintEndJob();
  }
  PrintExit(hwndMdi);

  ESL_FreeMem(lpBuf);
}

// from menu\menutree.c
static HDC DOM_PrintStart(HWND hWnd, LPSTR lpBuf, BOOL bTrunc)
{
  char  work[MAX_STRINGLEN];
  LPSTR lp;
  UINT  uiSpaceCount;
  HDC   hDCPrinter;

  for (uiSpaceCount = 0 , lp = work;*lpBuf &&  uiSpaceCount<2;lpBuf++) {
#ifdef DOUBLEBYTE
	if (IsDBCSLeadByte(*lpBuf))
		*lp++=*lpBuf++;
		else
#endif /* doublebyte */
    if (bTrunc && *lpBuf==' ')
      uiSpaceCount++;
    *lp++=*lpBuf;
  }
  *lp=0;
  hDCPrinter = PrintInit(hWnd,work);
  if (hDCPrinter) {
    PrintSetHeaderFileName(work);
    PrintStartJob(work, NULL);
  }
  return hDCPrinter;
}

// from menu\menutree.c
static void DOM_OutputAll(HDC hDCPrinter,HWND hwnd,HWND hList,LPSTR lpTitle,BOOL bSelOnly, LPDOMDATA lpDomData)
{
  POINT             ptPageSize;
  POINT             ptFontSize;

  SetMapMode(hDCPrinter, MM_TEXT);

  PrintSetFont(GetlpFontInfoFromDOM(hwnd, lpDomData));

  if (DOM_GetPrinterExtents(hDCPrinter,&ptPageSize,&ptFontSize))
  {
  LMDRAWITEMSTRUCT  lm;
  UINT              uiMargin=ptFontSize.y/2;
  UINT              uiPage=1;

  if (!uiMargin)
    uiMargin=1;

  lm.hDC = hDCPrinter;
  lm.uiIndentSize=ptFontSize.x;

  lm.rcItem.left=ptFontSize.x;
  lm.rcItem.right=ptPageSize.x-ptFontSize.x;

  lm.rcItem.top=(ptFontSize.y*2);

  lm.rcItem.bottom=lm.rcItem.top+ptFontSize.y;

  lm.rcPaint= lm.rcItem;

  lm.uiItemState= bSelOnly ? LMPRINTSELECTONLY : 0;

  StartPage(hDCPrinter);
  SetMapMode(hDCPrinter, MM_TEXT);

  PrintSetFont(GetlpFontInfoFromDOM(hwnd, lpDomData));

  SetMapMode(hDCPrinter, MM_TEXT);

  DOM_PrintHeader(hDCPrinter,lpTitle,ptPageSize,ptFontSize,uiPage);

  if (SendMessage(hList,LM_PRINT_GETFIRST,(WPARAM) TRUE,(LPARAM) (LPVOID) (LPLMDRAWITEMSTRUCT)&lm))
  {
    do
    {
  lm.rcItem.top=lm.rcItem.bottom+uiMargin;
  lm.rcItem.bottom+=ptFontSize.y+uiMargin;
  if (lm.rcItem.bottom>ptPageSize.y)
  {
    lm.rcItem.top=(ptFontSize.y*2);
    lm.rcItem.bottom=lm.rcItem.top+ptFontSize.y;
    if (EndPage(hDCPrinter)<=0 || StartPage(hDCPrinter)<=0)
    break;
    SetMapMode(hDCPrinter, MM_TEXT);

    PrintSetFont(GetlpFontInfoFromDOM(hwnd, lpDomData));

    SetMapMode(hDCPrinter, MM_TEXT);
    uiPage++;
    DOM_PrintHeader(hDCPrinter,lpTitle,ptPageSize,ptFontSize,uiPage);
  }
  lm.rcPaint=lm.rcItem;
    }while (SendMessage(hList,LM_PRINT_GETNEXT,(WPARAM) TRUE,(LPARAM) (LPVOID) (LPLMDRAWITEMSTRUCT)&lm));
  }
  }
}

// from menu\menutree.c
static BOOL DOM_GetPrinterExtents(HDC hDCPrinter,LPPOINT ptPageSize,LPPOINT ptFontSize)
{
  POINT ptOffset;
  DWORD dwExt;
  char c='0';

  dwExt=GetTextExtent(hDCPrinter,(LPSTR) &c,1);
  ptFontSize->x=LOWORD(dwExt)*4;
  ptFontSize->y=HIWORD(dwExt)+7;

  if (Escape(hDCPrinter, GETPHYSPAGESIZE, 0, NULL, (void FAR*) ptPageSize )<0)
  return FALSE;

  if (Escape(hDCPrinter, GETPRINTINGOFFSET, 0, NULL, (void FAR*) &ptOffset )>0)
  {
  ptPageSize->x-=(ptOffset.x*2);
  ptPageSize->y-=(ptOffset.y*2);
  }
  if (ptFontSize->y>=ptPageSize->y)
  return FALSE;
  return TRUE;
}

// from menu\menutree.c
static void DOM_PrintHeader(HDC hDCPrinter,LPSTR lpTitle,POINT ptPageSize,POINT ptFontSize,UINT uiPage)
{
  int  uiVertPos;
  int  uiHorzPos;
  char resStr[40];
  char work[80];
  UINT OldAlign;
  SIZE  xSize;

  // Print Title
  if (lpTitle)
    // Changed Emb 16/3/95 : added margin - TO BE FINISHED
    // Original code : TextOut(hDCPrinter,0,0,lpTitle,lstrlen(lpTitle));
    // TextOut(hDCPrinter, 0, 18, lpTitle, lstrlen(lpTitle));

   // UKS modifies March 4' 1996 >>
  {
    OldAlign = GetTextAlign (hDCPrinter);  
    SetTextAlign  (hDCPrinter, TA_LEFT);
    TextOut(hDCPrinter, 0, 18, lpTitle, lstrlen(lpTitle));
    SetTextAlign  (hDCPrinter, OldAlign);
  }
  // UKS <<

  // Print page number

  LoadString(hResource, IDS_DOMDOC_PRINTPAGENUM, resStr, sizeof(resStr));
  wsprintf(work,resStr,uiPage);

  uiHorzPos= ptPageSize.x-LOWORD(GetTextExtent(hDCPrinter,work,lstrlen(work)));

  // Changed Emb 16/3/95 : added margin - TO BE FINISHED
  // Original code : TextOut(hDCPrinter,uiHorzPos-1,0,work,lstrlen(work));
  // TextOut(hDCPrinter, uiHorzPos - 1 - 3*18, 18, work, lstrlen(work));

  // UKS modifis March 4' 1996 >>
#ifdef WIN32
  if (GetTextExtentPoint32(hDCPrinter, (LPCTSTR)work, x_strlen (work), &xSize))
#else
  if (GetTextExtentPoint  (hDCPrinter, (LPCSTR)work, x_strlen (work), &xSize))
#endif
  {
    // UKS modifies March 4' 1996 >>
    int xWidth; 
    xWidth = GetDeviceCaps (hDCPrinter, HORZRES);
    TextOut(hDCPrinter, xWidth - xSize.cx, 18, work, lstrlen(work));
  }
  else
  {
    // Old codes
    TextOut(hDCPrinter, uiHorzPos - 1 - 3*18, 18, work, lstrlen(work));
  }
  // UKS <<


  {  
    int xWidth; // UKS 
    xWidth = GetDeviceCaps (hDCPrinter, HORZRES);
    uiVertPos=MulDiv(ptFontSize.y,4,3);
    MoveToEx(hDCPrinter,0,uiVertPos, NULL);
    LineTo(hDCPrinter,/*ptPageSize.x*/ xWidth,uiVertPos);
  }
}

static BOOL GetSelectedTable(LPDOMDATA lpDomData, LPUCHAR lpBufName, LPUCHAR lpBufOwner)
{
  DWORD   dwItem;
  dwItem = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData,dwItem ) == OT_TABLE) {
    LPTREERECORD        lpRecord;
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    lstrcpy(lpBufName,lpRecord->objName);
    lstrcpy(lpBufOwner,lpRecord->ownerName);
    return TRUE;
  }
  else
    return FALSE;
}

static BOOL GetSelectedIndex(LPDOMDATA lpDomData, LPUCHAR lpBufName, LPUCHAR lpBufOwner)
{
  DWORD   dwItem;
  dwItem = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData,dwItem ) == OT_INDEX) 
  {
    LPTREERECORD lpRecord;
    lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA, 0, (LPARAM)dwItem);
    lstrcpy(lpBufName,lpRecord->objName);
    lstrcpy(lpBufOwner,lpRecord->ownerName);
    return TRUE;
  }
  else
    return FALSE;
}

BOOL VerifyDB (HWND hwndMdi, LPDOMDATA lpDomData)
{
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  VERIFYDBPARAMS  verifydb;

  // initialize structure
  memset(&verifydb, 0, sizeof(verifydb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, verifydb.DBName))
    return FALSE;

  // Check if the current branch  is a table type and Pick Table name.
  if (GetSelectedTable(lpDomData, verifydb.szDisplayTableName, BufTblOwner))
    verifydb.bStartSinceTable = TRUE;
  else
    verifydb.bStartSinceTable = FALSE;

  // Call dialog box
  return (BOOL)DlgVerifyDB (hwndMdi, &verifydb);
}


BOOL Checkpoint (HWND hwndMdi, LPDOMDATA lpDomData)
{
  CHKPT  chkpt;
  int ires;
  UCHAR    DBOwner[MAXOBJECTNAME];
  UCHAR    UserName[MAXOBJECTNAME];
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  UCHAR    BufTblName[MAXOBJECTNAME];
  UCHAR    RmcmdOwnerName[MAXOBJECTNAME];
  UCHAR    szMessageBuf[BUFSIZE+MAXOBJECTNAME];
  UCHAR    bufRs[BUFSIZE];
  LPUCHAR  vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);

  // initialize structure
  memset(&chkpt, 0, sizeof(chkpt));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, chkpt.DBName))
    return FALSE;

  ires=DOMGetDBOwner(lpDomData->psDomNode->nodeHandle,chkpt.DBName, DBOwner);
  if (ires!=RES_SUCCESS)
    return FALSE;

  ShowHourGlass();
  if (GetOIVers() >= OIVERS_30) {
    ires = MfcGetRmcmdObjects_launcher ( vnodeName , RmcmdOwnerName);
    if (!ires) {
        LoadString(hResource, IDS_E_RMCMD_USER_EXECUTING, bufRs, sizeof(bufRs));
        MessageBox(hwndMdi,
                   bufRs,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }
  }
  else
  {
     // Check the current owner of the Rmcmd Object
    ires = GetRmcmdObjects_Owner(vnodeName, RmcmdOwnerName);
    RemoveHourGlass();
    if (ires == RES_ERR ) {
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
    return FALSE;
    }
  }
  // check the user is ingres or the DBA
  DBAGetUserName(vnodeName, UserName);
  if ( x_stricmp(UserName, RmcmdOwnerName) && x_stricmp(UserName,DBOwner)) {
    LoadString(hResource, IDS_E_RUNONLYBY_INGRES_OR_DBA, bufRs, sizeof(bufRs));
    wsprintf(szMessageBuf, bufRs, RmcmdOwnerName);
    MessageBox(hwndMdi,
               szMessageBuf, // "This Command Can Only Be Launched by %s or the DBA."
               NULL,
               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    return FALSE;
  }
  // Check if the current object is a table and Pick Table name.
  if (GetSelectedTable(lpDomData, BufTblName, BufTblOwner))
  {
    StringWithOwner(BufTblName,BufTblOwner, chkpt.szDisplayTableName);
    chkpt.bStartSinceTable = TRUE;
  }
  else
    chkpt.bStartSinceTable = FALSE;

  // Call dialog box
  return (DlgCheckpoint(hwndMdi, &chkpt));
}

BOOL RollForward   (HWND hwndMdi, LPDOMDATA lpDomData)
{
  ROLLFWDPARAMS  rollfwd;
  BOOL           bRet;
  int      ires;
  UCHAR    DBOwner[MAXOBJECTNAME];
  UCHAR    UserName[MAXOBJECTNAME];
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  UCHAR    BufTblName[MAXOBJECTNAME];
  UCHAR    RmcmdOwnerName[MAXOBJECTNAME];
  UCHAR    szMessageBuf[BUFSIZE+MAXOBJECTNAME];
  UCHAR    bufRs[BUFSIZE];
  LPUCHAR  vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);

  // initialize structure
  memset(&rollfwd, 0, sizeof(rollfwd));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, rollfwd.DBName))
    return FALSE;

  ires=DOMGetDBOwner(lpDomData->psDomNode->nodeHandle,rollfwd.DBName, DBOwner);
  if (ires!=RES_SUCCESS)
    return FALSE;

  ShowHourGlass();
  if (GetOIVers() >= OIVERS_30) {
    ires = MfcGetRmcmdObjects_launcher ( vnodeName , RmcmdOwnerName);
    if (!ires) {
        LoadString(hResource, IDS_E_RMCMD_USER_EXECUTING, bufRs, sizeof(bufRs));
        MessageBox(hwndMdi,
                   bufRs,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }
  }
  else
  {
     // Check the current owner of the Rmcmd Object
    ires = GetRmcmdObjects_Owner(vnodeName, RmcmdOwnerName);
    RemoveHourGlass();
    if (ires == RES_ERR ) {
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
      return FALSE;
    }
  }
  // check the user is ingres or the DBA
  DBAGetUserName(vnodeName, UserName);
  if ( x_stricmp(UserName, RmcmdOwnerName) && x_stricmp(UserName,DBOwner)) {
    LoadString(hResource, IDS_E_RUNONLYBY_INGRES_OR_DBA, bufRs, sizeof(bufRs));
    wsprintf(szMessageBuf, bufRs, RmcmdOwnerName);
    MessageBox(hwndMdi,
               szMessageBuf, // "This Command Can Only Be Launched by %s or the DBA."
               NULL,
               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    return FALSE;
  }

  // Check if the current object is a table and Pick Table name.
  if (GetSelectedTable(lpDomData, BufTblName, BufTblOwner))
  {
    StringWithOwner(BufTblName,BufTblOwner, rollfwd.szDisplayTableName);
    rollfwd.bStartSinceTable = TRUE;
  }
  else
    rollfwd.bStartSinceTable = FALSE;

  // Call dialog box
  rollfwd.node = lpDomData->psDomNode->nodeHandle;
  bRet = (BOOL)DlgRollForward (hwndMdi, &rollfwd);

  return bRet;
}

BOOL DisplayStat   (HWND hwndMdi, LPDOMDATA lpDomData)
{
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  STATDUMPPARAMS  statdump;
  DWORD   dwItem;
  int nObjType;

  // initialize structure
  memset(&statdump, 0, sizeof(statdump));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, statdump.DBName))
    return FALSE;

  dwItem = GetCurSel(lpDomData);
  nObjType = GetItemObjType(lpDomData,dwItem);
  statdump.m_nObjectType = nObjType;
  switch (nObjType)
  {
  case OT_DATABASE:
    break;
  case OT_TABLE:
    // Check if the current branch  is a table type and Pick Table name.
    if (GetSelectedTable(lpDomData, statdump.szDisplayTableName, BufTblOwner))
    {
      StringWithOwner(statdump.szDisplayTableName,BufTblOwner, statdump.szCurrentTableName);
      statdump.bStartSinceTable = TRUE;
    }
    break;
  case OT_INDEX:
    // Check if the current branch  is a table type and Pick Index name.
    if (GetSelectedIndex(lpDomData, statdump.tchszDisplayIndexName, BufTblOwner))
    {
      StringWithOwner(statdump.tchszDisplayIndexName, BufTblOwner, statdump.tchszCurrentIndexName);
      statdump.bStartSinceIndex = TRUE;
    }
    break;
  }

  // Call dialog box
  return (BOOL)DlgDisplayStatistics (hwndMdi, &statdump);
}


//
// Calls the dialog box that allows to perform sysmod operation
//
//static BOOL NEAR SysmodOperation(HWND hwndMdi, LPDOMDATA lpDomData)
//{
//  BOOL    bRet;
//  SYSMOD  sysmod;
//  DWORD   dwItem;
//
//  dwItem = GetCurSel(lpDomData);
//
//  // check we are on a database item
//  if (GetCurSelObjType(lpDomData) != OT_DATABASE)
//    return FALSE;
//
//  // initialize sysmod structure with database name
//  memset(&sysmod, 0, sizeof(sysmod));
//  x_strcpy(sysmod.szDBName, GetObjName(lpDomData, dwItem));
//
//  // call dialog box
//  bRet = DlgSysMod(hwndMdi, &sysmod);
//  if (bRet) {
//    // TO BE FINISHED : WHAT CODE THEN ???
//    //MessageBox(hwndMdi, "rmcmd command needed on server", "Sysmod", MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//  }
//  return bRet;
//}

//
// Calls the dialog box that allows to perform sysmod operation
//
BOOL SysmodOperation(HWND hwndMdi, LPDOMDATA lpDomData)
{
  SYSMOD  sysmod;
  UCHAR   UserName[MAXOBJECTNAME];
  UCHAR   RmcmdOwnerName[MAXOBJECTNAME];
  UCHAR   bufRs[BUFSIZE];
  UCHAR   szMessageBuf[BUFSIZE+MAXOBJECTNAME];
  LPUCHAR   vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
  int ires;

  // initialize structure
  memset(&sysmod, 0, sizeof(sysmod));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, sysmod.szDBName))
    return FALSE;

  ShowHourGlass();
  if (GetOIVers() >= OIVERS_30) {
    ires = MfcGetRmcmdObjects_launcher ( vnodeName , RmcmdOwnerName);
    if (!ires) {
        LoadString(hResource, IDS_E_RMCMD_USER_EXECUTING, bufRs, sizeof(bufRs));
        MessageBox(hwndMdi,
                   bufRs,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }
  }
  else
  {
    // Check the current owner of the Rmcmd Object
      ires = GetRmcmdObjects_Owner(vnodeName, RmcmdOwnerName);
    RemoveHourGlass();
    if (ires == RES_ERR ) {
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
      return FALSE;
    }
  }

  // check the user is the Owner of the rmcmd object
  DBAGetUserName(vnodeName, UserName);
  if (x_stricmp(UserName, RmcmdOwnerName)) {
    LoadString(hResource, IDS_E_RUNONLYBY_INGRES, bufRs, sizeof(bufRs));
    wsprintf(szMessageBuf, bufRs, RmcmdOwnerName);

    MessageBox(hwndMdi,
               szMessageBuf,   // This Command Can Only Be Launched by the %s Account
               NULL,
               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    return FALSE;
  }

  // call dialog box
  return(DlgSysMod(hwndMdi, &sysmod));
}

//
//  Calls the dialog box that manages the alterDB command
//
BOOL AlterDB(HWND hwndMdi, LPDOMDATA lpDomData)
{
  ALTERDB alterdb;
  UCHAR   UserName[MAXOBJECTNAME];
  UCHAR   RmcmdOwnerName[MAXOBJECTNAME];
  UCHAR   bufRs[BUFSIZE];
  UCHAR   szMessageBuf[BUFSIZE+MAXOBJECTNAME];
  int ires;
  LPUCHAR   vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);

  // initialize structure
  memset(&alterdb, 0, sizeof(alterdb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, alterdb.DBName))
    return FALSE;

  ShowHourGlass();
  if (GetOIVers() >= OIVERS_30) {
    ires = MfcGetRmcmdObjects_launcher ( vnodeName , RmcmdOwnerName);
    if (!ires) {
        LoadString(hResource, IDS_E_RMCMD_USER_EXECUTING, bufRs, sizeof(bufRs));
        MessageBox(hwndMdi,
                   bufRs,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }
  }
  else
  {
     // Check the current owner of the Rmcmd Object
    ires = GetRmcmdObjects_Owner(vnodeName, RmcmdOwnerName);
    RemoveHourGlass();
    if (ires == RES_ERR ) {
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
      return FALSE;
    }
  }
  // check the user is ingres
  DBAGetUserName(vnodeName, UserName);
  if ( x_stricmp(UserName, RmcmdOwnerName)) {
    LoadString(hResource, IDS_E_RUNONLYBY_INGRES, bufRs, sizeof(bufRs));
    wsprintf(szMessageBuf, bufRs, RmcmdOwnerName);
    MessageBox(hwndMdi,
               szMessageBuf,   // This Command Can Only Be Launched by the %s Account
               NULL,
               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    return FALSE;
  }

  // call dialog box
  return DlgAlterDB(hwndMdi, &alterdb);
}


//
// Populate a database
//
BOOL Populate(HWND hwndMdi, LPDOMDATA lpDomData)
{
	int nError;
	DWORD   dwCurSel;
	int     CurItemObjType;
	LPTREERECORD lpRecord;
	char szgateway[MAXOBJECTNAME*4];
	IIASTRUCT iiparam;
	FINDCURSOR findcursor;
	BOOL bLenOK = TRUE;
	UINT nExistOpenCursor = 0;
	const int nBuffLen = 64;
	TCHAR tchszNode[256];
	LPUCHAR  vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
	BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);
	lstrcpy (tchszNode, vnodeName);
	RemoveGWNameFromString((LPUCHAR)tchszNode);
	RemoveConnectUserFromString((LPUCHAR)tchszNode);

	// initialize structure
	memset(&findcursor, 0, sizeof(findcursor));
	memset(&iiparam, 0, sizeof(iiparam));
	iiparam.bLoop    = FALSE;
	iiparam.bBatch   = FALSE;
	iiparam.bNewTable= FALSE;
	iiparam.bDisableLoadParam  = TRUE;
	iiparam.bDisableTreeControl= TRUE;
	iiparam.bStaticTreeData    = TRUE;
	iiparam.nSessionStart = MAXSESSIONS;
	wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes Ingres Import Assistant");

	CMmbstowcs((LPCTSTR)tchszNode, iiparam.wczNode, nBuffLen);
	CMmbstowcs((LPCTSTR)szgateway, iiparam.wczServerClass, nBuffLen);

	// get information on current item
	dwCurSel = GetCurSel(lpDomData);
	CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
	lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

	switch (CurItemObjType) 
	{
	case OT_DATABASE:
		CMmbstowcs((LPCTSTR)lpRecord->objName, iiparam.wczDatabase, nBuffLen);
		iiparam.bDisableTreeControl= FALSE;
		break;
	case OT_TABLE:
		// DATABASE NAME:
		CMmbstowcs((LPCTSTR)lpRecord->extra, iiparam.wczDatabase, nBuffLen);
		CMmbstowcs((LPCTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner (lpRecord->objName)), iiparam.wczTable, nBuffLen);
		CMmbstowcs((LPCTSTR)lpRecord->ownerName, iiparam.wczTableOwner, nBuffLen);
		break;
	default:
		return FALSE;
	}

#if defined (_FINDnCLOSE_OPENCURSOR)
	if (IsReadLockActivated())
	{
		REQUESTMDIINFO* pArrayInfo = NULL;
		int nInfoSize = 0;
		findcursor.bCloseCursor = FALSE;
		lstrcpy (findcursor.tchszNode,       (LPCTSTR)tchszNode);
		lstrcpy (findcursor.tchszServer,     (LPCTSTR)szgateway);
		lstrcpy (findcursor.tchszDatabase,   (LPCTSTR)lpRecord->extra);
		lstrcpy (findcursor.tchszTable,      (LPCTSTR)RemoveDisplayQuotesIfAny(StringWithoutOwner (lpRecord->objName)));
		lstrcpy (findcursor.tchszTableOwner, (LPCTSTR)lpRecord->ownerName);
		nExistOpenCursor = GetExistOpenCursor (&findcursor, &pArrayInfo, &nInfoSize);
		if (nInfoSize > 0 && pArrayInfo != NULL)
			free (pArrayInfo);

		if ((nExistOpenCursor & OPEN_CURSOR_SQLACT))
		{
			MessageBox(hwndMdi , ResourceString ((UINT)IDS_EXIST_OPENCURSOR_IN_SQLTEST), NULL, MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}
	}
#endif
	SetDisplayVdbaHelp (FALSE);
#if defined (EDBC)
	MessageBox (hwndMdi, "Populate using IIA is note available for EDBC version", NULL, MB_ICONEXCLAMATION | MB_OK);
	return FALSE;
#endif
	nError = Ingres_ImportAssistantWithParam(hwndMdi, &iiparam);
	SetDisplayVdbaHelp (TRUE);
	switch (nError)
	{
	case 1:
		return TRUE;
	case 2:
		return FALSE;
	case 3:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_INITIALIZE_COM), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 4:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_SVRIIA_NOT_REGISTERED), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 5:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_CREATE_AS_AGREGATION), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 6:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_QUERY_INTERFACE), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	default:
		return FALSE;
	}

	return TRUE;
}

BOOL Export(HWND hwndMdi, LPDOMDATA lpDomData)
{
	int nError;
	DWORD   dwCurSel;
	int     CurItemObjType;
	LPTREERECORD lpRecord;
	char szgateway[MAXOBJECTNAME*4];
	WCHAR wchStatement[MAXOBJECTNAME*8];
	WCHAR wchTable[MAXOBJECTNAME*2];
	WCHAR wchTableOwner[MAXOBJECTNAME*2];
	IEASTRUCT iiparam;
	FINDCURSOR findcursor;
	BOOL bLenOK = TRUE;
	UINT nExistOpenCursor = 0;
	const int nBuffLen = MAXOBJECTNAME;
	TCHAR tchszNode[MAXOBJECTNAME*4];
	LPUCHAR  vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
	BOOL bHasGWSuffix = GetGWClassNameFromString(vnodeName, szgateway);
	lstrcpy (tchszNode, vnodeName);
	RemoveGWNameFromString((LPUCHAR)tchszNode);
	RemoveConnectUserFromString((LPUCHAR)tchszNode);

	// initialize structure
	memset(&findcursor, 0, sizeof(findcursor));
	memset(&iiparam, 0, sizeof(iiparam));
	iiparam.bLoop    = FALSE;
	iiparam.bSilent  = FALSE;
	iiparam.nSessionStart = MAXSESSIONS;
	wcscpy (iiparam.wczSessionDescription, L"Ingres Visual DBA invokes Ingres Export Assistant");

	CMmbstowcs((LPCTSTR)tchszNode, iiparam.wczNode, nBuffLen);
	CMmbstowcs((LPCTSTR)szgateway, iiparam.wczServerClass, nBuffLen);

	// get information on current item
	dwCurSel = GetCurSel(lpDomData);
	CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
	lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

	switch (CurItemObjType) 
	{
	case OT_DATABASE:
		CMmbstowcs((LPCTSTR)lpRecord->objName, iiparam.wczDatabase, nBuffLen);
		break;
	case OT_TABLE:
		// DATABASE NAME:
		CMmbstowcs((LPCTSTR)lpRecord->extra, iiparam.wczDatabase, nBuffLen);
		CMmbstowcs((LPCTSTR)QuoteIfNeeded(StringWithoutOwner (lpRecord->objName)), wchTable, nBuffLen);
		CMmbstowcs((LPCTSTR)QuoteIfNeeded(lpRecord->ownerName), wchTableOwner, nBuffLen);
		swprintf (wchStatement, sizeof(wchStatement), L"select * from %s.%s", wchTableOwner, wchTable);
		iiparam.lpbstrStatement = SysAllocString(wchStatement);
		break;
	default:
		return FALSE;
	}


	SetDisplayVdbaHelp (FALSE);
#if defined (EDBC)
	if (iiparam.lpbstrStatement)
		SysFreeString(iiparam.lpbstrStatement);
	MessageBox (hwndMdi, "Export using IEA is note available for EDBC version", NULL, MB_ICONEXCLAMATION | MB_OK);
	return FALSE;
#endif
	nError = Ingres_ExportAssistantWithParam(hwndMdi, &iiparam);
	if (iiparam.lpbstrStatement)
		SysFreeString(iiparam.lpbstrStatement);

	SetDisplayVdbaHelp (TRUE);
	switch (nError)
	{
	case 1:
		return TRUE;
	case 2:
		return FALSE;
	case 3:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_INITIALIZE_COM), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 4:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_SVRIIA_NOT_REGISTERED), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 5:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_CREATE_AS_AGREGATION), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	case 6:
		MessageBox(hwndMdi , ResourceString ((UINT)IDS_FAILED_2_QUERY_INTERFACE), NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	default:
		return FALSE;
	}

	return TRUE;

}


//
// Audit a database
//
BOOL Audit(HWND hwndMdi, LPDOMDATA lpDomData)
{
  AUDITDBPARAMS auditdb;
  int ires;
  UCHAR    DBOwner[MAXOBJECTNAME];
  UCHAR    UserName[MAXOBJECTNAME];
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  UCHAR    RmcmdOwnerName[MAXOBJECTNAME];
  UCHAR    szMessageBuf[BUFSIZE+MAXOBJECTNAME];
  UCHAR    bufRs[BUFSIZE];
  LPUCHAR  vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);

  // initialize structure
  memset(&auditdb, 0, sizeof(auditdb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, auditdb.DBName))
    return FALSE;

  ires=DOMGetDBOwner(lpDomData->psDomNode->nodeHandle,auditdb.DBName, DBOwner);
  if (ires!=RES_SUCCESS)
    return FALSE;

  ShowHourGlass();
   // Check the current owner of the Rmcmd Object
  if (GetOIVers() >= OIVERS_30) {
    ires = MfcGetRmcmdObjects_launcher ( vnodeName , RmcmdOwnerName);
    if (!ires) {
        LoadString(hResource, IDS_E_RMCMD_USER_EXECUTING, bufRs, sizeof(bufRs));
        MessageBox(hwndMdi,
                   bufRs,
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
        return FALSE;
    }
  }
  else
  {
    ires = GetRmcmdObjects_Owner(vnodeName, RmcmdOwnerName);
    RemoveHourGlass();
    if (ires == RES_ERR ) {
      MessageWithHistoryButton(GetFocus(),ResourceString(IDS_ERR_ACCESS_RMCMD_OBJ) ) ;
      return FALSE;
    }
  }
  // check the user is ingres or the DBA
  DBAGetUserName(vnodeName, UserName);
  if ( x_stricmp(UserName, RmcmdOwnerName) && x_stricmp(UserName,DBOwner)) {
    LoadString(hResource, IDS_E_RUNONLYBY_INGRES_OR_DBA, bufRs, sizeof(bufRs));
    wsprintf(szMessageBuf, bufRs, RmcmdOwnerName);
    MessageBox(hwndMdi,
               szMessageBuf, // "This Command Can Only Be Launched by %s or the DBA."
               NULL,
               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
    return FALSE;
  }

  // Check if the current object is a table and Pick Table name.
  if (GetSelectedTable(lpDomData, auditdb.szDisplayTableName, BufTblOwner))
  {
    StringWithOwner(auditdb.szDisplayTableName,BufTblOwner, auditdb.szCurrentTableName);
    auditdb.bStartSinceTable = TRUE;
  }
  else
    auditdb.bStartSinceTable = FALSE;

  // Call dialog box
  return (BOOL)DlgAuditDB (hwndMdi, &auditdb);
}

// Verify number of column key in index
//
BOOL VerifyNumKeyColumn(LPTSTR lpVnode, LPINDEXPARAMS lpIndexPar)
{
  int iResult, dummySesHndl;
  BOOL bRet = TRUE;

  iResult = GetDetailInfo ((LPUCHAR)lpVnode, OT_INDEX, lpIndexPar, FALSE, &dummySesHndl);
  if (iResult != RES_SUCCESS)
    bRet = FALSE;
  else
  {
    if (CountListItems(lpIndexPar->lpidxCols)<2)
    {
      //
      // Statistics cannot be generated on this index since it has only
      // 1 key column. This operation is used to generate a composite histogram.
      TS_MessageBox(TS_GetFocus(), ResourceString(IDS_ERR_ONLY_WITH_TWO_KEY_COL),
                    ResourceString(IDS_TITLE_WARNING), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
      bRet = FALSE;
    }
  }
  // liberate detail structure
  FreeAttachedPointers (lpIndexPar, OT_INDEX);
  return bRet;
}

//
// Generate statistics on a database
//
BOOL GenStat(HWND hwndMdi, LPDOMDATA lpDomData)
{
  UCHAR BufTblOwner[MAXOBJECTNAME];
  OPTIMIZEDBPARAMS    optimizedb;
  DWORD   dwItem;
  int nObjType;

  memset(&optimizedb, 0, sizeof(optimizedb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, optimizedb.DBName))
    return FALSE;

  dwItem = GetCurSel(lpDomData);
  nObjType = GetItemObjType(lpDomData,dwItem);
  optimizedb.m_nObjectType = nObjType;
  switch (nObjType)
  {
  case OT_DATABASE:
    break;
  case OT_TABLE:
    // Verify if the branch is OT_TABLE type.
    if (GetSelectedTable(lpDomData, optimizedb.szDisplayTableName, BufTblOwner))
    {
      optimizedb.bSelectTable = TRUE;
      optimizedb.bSelectColumn= FALSE;
      StringWithOwner(optimizedb.szDisplayTableName, BufTblOwner, optimizedb.TableName);
    }
    else
    {
      optimizedb.bSelectTable = FALSE;
      optimizedb.bSelectColumn= FALSE;
    }
    break;
  case OT_INDEX:
    // Verify if the branch is OT_INDEX type.
    if (GetSelectedIndex(lpDomData, optimizedb.tchszDisplayIndexName, BufTblOwner))
    {
      INDEXPARAMS IndexPar;
      memset (&IndexPar, 0, sizeof (IndexPar));
      x_strcpy ((char *)IndexPar.DBName, optimizedb.DBName);
      x_strcpy ((char *)IndexPar.objectname,optimizedb.tchszDisplayIndexName);
      x_strcpy ((char *)IndexPar.objectowner,BufTblOwner);
      x_strcpy ((char *)IndexPar.TableName, optimizedb.TableName);
      x_strcpy ((char *)IndexPar.TableOwner,optimizedb.TableOwner);
      if (!VerifyNumKeyColumn(GetVirtNodeName (lpDomData->psDomNode->nodeHandle), &IndexPar))
        return FALSE;

      optimizedb.bSelectIndex = TRUE;
      StringWithOwner(optimizedb.tchszDisplayIndexName, BufTblOwner, optimizedb.tchszIndexName);
    }
    break;
  }
  return (BOOL)DlgOptimizeDB (hwndMdi, &optimizedb);
}

extern BOOL MfcCopyDB(HWND hwndDoc); // Childframe.cpp

//
// Calls the dialog box that allows to perform copydb operation
//
BOOL CopyDBOperation(HWND hwndMdi, LPDOMDATA lpDomData)
{
  UCHAR    BufTblOwner[MAXOBJECTNAME];
  COPYDB  copydb;

  if (lpDomData->ingresVer>=OIVERS_26)
    return (MfcCopyDB(hwndMdi));

  // initialize structure
  memset(&copydb, 0, sizeof(copydb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, copydb.DBName))
    return FALSE;

  // Check if the current object is a table and Pick Table name.
  if (GetSelectedTable(lpDomData, copydb.szDisplayTableName, BufTblOwner))
  {
    StringWithOwner(copydb.szDisplayTableName,BufTblOwner,
                    copydb.szCurrentTableName);
    copydb.bStartSinceTable = TRUE;
  }
  else
    copydb.bStartSinceTable = FALSE;

  // call dialog box
  return(DlgCopyDB(hwndMdi, &copydb));
}

//
// Calls the dialog box that allows to perform unloaddb operation
//
BOOL UnloadDBOperation(HWND hwndMdi, LPDOMDATA lpDomData)
{
   COPYDB  copydb;

  // initialize structure
  memset(&copydb, 0, sizeof(copydb));

  // double-check we are on a database direct child,
  // and pick database name
  if (!HasTrueParentDB(lpDomData, copydb.DBName))
    return FALSE;

   // call dialog box
   return(DlgUnLoadDB(hwndMdi, &copydb));
}

//--------------------------------------------------------------------------
//
// Clipboard-related functions
//
//--------------------------------------------------------------------------

static UINT NEAR GetDetailsSize(int objType);
static int  NEAR GetDetailsOnObject(int nodeHandle, int objType, LPTREERECORD lpRecord, void *lpMem);
static int  NEAR CreateObjFromClipboardData(HWND hwndMdi, LPDOMDATA lpDomData, void *lpMem, int size, int objType);
static void NEAR ErrorClipboardCopyDomObj(int reserr, BOOL bOtherErr);
static void NEAR ErrorClipboardPasteDomObj(int reserr, BOOL bOtherErr);

#include "msghandl.h" // Error messages management

//
//  Displays an error message for erroneous copy Dom object into clipboard
//
static void NEAR ErrorClipboardCopyDomObj(int reserr, BOOL bOtherErr)
{
  if (bOtherErr)
    reserr = RES_ERR;
  ErrorMessage (IDS_ERR_CLIPBOARD_DOM_COPY , reserr);
}

//
//  Displays an error message for erroneous copy Dom object into clipboard
//
static void NEAR ErrorClipboardPasteDomObj(int reserr, BOOL bOtherErr)
{
  if (bOtherErr)
    reserr = RES_ERR;
  ErrorMessage (IDS_ERR_CLIPBOARD_DOM_PASTE , reserr);
}

//
//  Copies the currently selected object characteristics in the clipboard,
//  or, if bQueryInfo is TRUE, simply gets all needed information for
//  use in drag/drop
//
// returns TRUE if successful
//
BOOL CopyDomObjectToClipboard(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bQueryInfo)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  int           objType;
  UINT          size;
  HANDLE        hMem;
  CHAR         *lpMem;
  int           res;

  // Pre-check
  if (IsCurSelObjStatic(lpDomData))
    return FALSE;                   // Cannot be copied!

  // Get object basic type, parenthood, ... with type solve on the fly
  objType = GetCurSelObjType(lpDomData);
  dwCurSel = GetCurSel(lpDomData);
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                          LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  objType = SolveType(lpDomData, lpRecord, FALSE);
  // Note that lpData->recType, parenthood, etc have NOT been updated
  // by calling SolveType since the last parameter was FALSE
  objType = GetBasicType(objType);

  if (bQueryInfo) {
    DomClipboardObjType = objType;
    DomClipboardObjOIVers = GetOIVers();
    return TRUE;
  }

  // ask for necessary size, and allocate as requested
  // NOTE : we don't use ESL_AllocMem : the clipboard REQUESTS GlobalAlloc
  size = GetDetailsSize(objType);
  if (!size) {
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
  if (!hMem) {
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  lpMem = GlobalLock(hMem);
  if (!lpMem) {
    GlobalFree(hMem);
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }

  // Fill allocated buffer
  res = GetDetailsOnObject(lpDomData->psDomNode->nodeHandle,
                           objType, lpRecord, lpMem);
  if (res != RES_SUCCESS) {
    FreeAttachedPointers ( lpMem, objType);
    ErrorClipboardCopyDomObj(res, FALSE);
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return FALSE;
  }

  // Pass the buffer handle to the clipboard
  GlobalUnlock(hMem);
  if (!OpenClipboard(hwndMdi)) {
    FreeAttachedPointers ( lpMem, objType);
    GlobalFree(hMem);
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  EmptyClipboard();
  SetClipboardData(wDomClipboardFormat, hMem);
  CloseClipboard();

  // update type of the last object copied in clipbd, for paste management
  // ( 2 variables: 1 for clear, 1 for paste)
  DomClipboardObjType         = objType;
  DomClipboardLastStoredType  = objType;
  DomClipboardObjOIVers       = GetOIVers();
  DomClipboardSrcNodeHandle   = lpDomData->psDomNode->nodeHandle;

  return TRUE;
}

BOOL CopyRPaneObjectToClipboard(HWND hwndMdi, int rPaneObjType, BOOL bQueryInfo, LPTREERECORD lpRecord)
{
  int           objType;
  UINT          size;
  HANDLE        hMem;
  CHAR         *lpMem;
  int           res;
  LPDOMDATA     lpDomData;

  objType = rPaneObjType;
  if (bQueryInfo) {
    DomClipboardObjType = objType;
    DomClipboardObjOIVers = GetOIVers();
    return TRUE;
  }

  // ask for necessary size, and allocate as requested
  // NOTE : we don't use ESL_AllocMem : the clipboard REQUESTS GlobalAlloc
  size = GetDetailsSize(objType);
  if (!size) {
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
  if (!hMem) {
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  lpMem = GlobalLock(hMem);
  if (!lpMem) {
    GlobalFree(hMem);
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }

  // Fill allocated buffer
  lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  if (!lpDomData) {
    ErrorClipboardCopyDomObj(0, FALSE);
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return FALSE;
  }
  res = GetDetailsOnObject(lpDomData->psDomNode->nodeHandle,
                           objType, lpRecord, lpMem);
  if (res != RES_SUCCESS) {
    ErrorClipboardCopyDomObj(res, FALSE);
    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return FALSE;
  }

  // Pass the buffer handle to the clipboard
  GlobalUnlock(hMem);
  if (!OpenClipboard(hwndMdi)) {
    GlobalFree(hMem);
    ErrorClipboardCopyDomObj(0, TRUE);
    return FALSE;
  }
  EmptyClipboard();
  SetClipboardData(wDomClipboardFormat, hMem);
  CloseClipboard();

  // update type of the last object copied in clipbd, for paste management
  // ( 2 variables: 1 for clear, 1 for paste)
  DomClipboardObjType         = objType;
  DomClipboardLastStoredType  = objType;
  DomClipboardObjOIVers       = GetOIVers();
  DomClipboardSrcNodeHandle   = lpDomData->psDomNode->nodeHandle;

  return TRUE;
}


//
// Returns the size of the structure that shoud be allocated
// prior to calling GetDetailsOnObject()
//
// returns 0 in case of error
//
static UINT NEAR GetDetailsSize(int objType)
{
  switch (objType) {
    case OT_GROUP:
      return sizeof(GROUPPARAMS);
    case OT_INTEGRITY:
      return sizeof(INTEGRITYPARAMS);
    case OT_LOCATION:
      return sizeof(LOCATIONPARAMS);
    case OT_PROCEDURE:
    case OT_SCHEMAUSER_PROCEDURE:
      return sizeof(PROCEDUREPARAMS);
    case OT_RULE:
      return sizeof(RULEPARAMS);
    case OT_USER:
      return sizeof(USERPARAMS);
    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      return sizeof(VIEWPARAMS);
    case OT_DBEVENT:
      return sizeof(DBEVENTPARAMS);
    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      return sizeof(TABLEPARAMS);
    case OT_INDEX:
      return sizeof(INDEXPARAMS);
    case OT_ROLE:
      return sizeof(ROLEPARAMS);
    case OT_SEQUENCE:
      return sizeof(SEQUENCEPARAMS);

    default:
      return 0;
  }
}

//
// Fills the previously allocated structure with detail info on object
//
// returns TRUE if successful
//
static int NEAR GetDetailsOnObject(int nodeHandle, int objType, LPTREERECORD lpRecord, void *lpMem)
{
  int     SesHndl;    // dummy session handle
  int     err;
  LPUCHAR lpVirtNode;
  LPOBJECTLIST lpObj;
  char szTxt[400];

  // prepare parameters
  switch (objType) {
    case OT_GROUP:
      // object name
      x_strcpy(((LPGROUPPARAMS)(lpMem))->ObjectName, lpRecord->objName);
      break;

    case OT_INTEGRITY:
      // integrity number
      ((LPINTEGRITYPARAMS)(lpMem))->number =lpRecord->complimValue;
      // base name : parent of level 0
      x_strcpy(((LPINTEGRITYPARAMS)(lpMem))->DBName, lpRecord->extra);
      // table name : parent of level 1
      x_strcpy(((LPINTEGRITYPARAMS)(lpMem))->TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->extra2)));
      // FOR SCHEMAS : parent table owner
      x_strcpy(((LPINTEGRITYPARAMS)(lpMem))->TableOwner, lpRecord->tableOwner);
      break;

    case OT_LOCATION:
      // object name
      x_strcpy(((LPLOCATIONPARAMS)(lpMem))->objectname, lpRecord->objName);
      break;

    case OT_PROCEDURE:
    case OT_SCHEMAUSER_PROCEDURE:
      // object name
      x_strcpy(((LPPROCEDUREPARAMS)(lpMem))->objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPPROCEDUREPARAMS)(lpMem))->DBName, lpRecord->extra);
      // FOR SCHEMAS : procedure owner
      x_strcpy(((LPPROCEDUREPARAMS)(lpMem))->objectowner, lpRecord->ownerName);
      break;

    case OT_RULE:
      // object name
      x_strcpy(((LPRULEPARAMS)(lpMem))->RuleName, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPRULEPARAMS)(lpMem))->DBName, lpRecord->extra);
      // table name : parent of level 1
      x_strcpy(((LPRULEPARAMS)(lpMem))->TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->extra2)));
      // FOR SCHEMAS : rule owner
      x_strcpy(((LPRULEPARAMS)(lpMem))->RuleOwner, lpRecord->ownerName);
      break;

    case OT_USER:
      // object name
      x_strcpy(((LPUSERPARAMS)(lpMem))->ObjectName, lpRecord->objName);
      break;

    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      // view name
      x_strcpy(((LPVIEWPARAMS)(lpMem))->objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPVIEWPARAMS)(lpMem))->DBName, lpRecord->extra);
      // FOR SCHEMAS : view owner
      x_strcpy(((LPVIEWPARAMS)(lpMem))->szSchema, lpRecord->ownerName);
      break;

    case OT_DBEVENT:
      // object name
      wsprintf(((LPDBEVENTPARAMS)(lpMem))->ObjectName,"%s.%s",QuoteIfNeeded(lpRecord->ownerName),QuoteIfNeeded(RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName))));
      //x_strcpy(((LPDBEVENTPARAMS)(lpMem))->ObjectName, lpRecord->objName);
      // base name : parent of level 0
      x_strcpy(((LPDBEVENTPARAMS)(lpMem))->DBName, lpRecord->extra);
      return RES_SUCCESS;   // No more details!
      break;

    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      // table name
      x_strcpy(((LPTABLEPARAMS)(lpMem))->objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPTABLEPARAMS)(lpMem))->DBName, lpRecord->extra);
      // FOR SCHEMAS : table owner
      x_strcpy(((LPTABLEPARAMS)(lpMem))->szSchema, lpRecord->ownerName);
      break;

    case OT_INDEX:
      // index name
      x_strcpy(((LPINDEXPARAMS)(lpMem))->objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPINDEXPARAMS)(lpMem))->DBName, lpRecord->extra);
      // table name : parent of level 1
      x_strcpy(((LPINDEXPARAMS)(lpMem))->TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->extra2)));
      // FOR SCHEMAS : index owner
      x_strcpy(((LPINDEXPARAMS)(lpMem))->objectowner, lpRecord->ownerName);
      break;

    case OT_ROLE:
      // object name
      x_strcpy(((LPROLEPARAMS)(lpMem))->ObjectName, lpRecord->objName);
      break;

    case OT_SEQUENCE:
      // object name
      x_strcpy(((LPSEQUENCEPARAMS)(lpMem))->objectname, RemoveDisplayQuotesIfAny(StringWithoutOwner(lpRecord->objName)));
      // base name : parent of level 0
      x_strcpy(((LPSEQUENCEPARAMS)(lpMem))->DBName, lpRecord->extra);
      // FOR SCHEMAS : Sequence owner
      x_strcpy(((LPSEQUENCEPARAMS)(lpMem))->objectowner, lpRecord->ownerName);
      break;

    default:
      return RES_ERR;
  }

  // call GetDetailInfo
  lpVirtNode = GetVirtNodeName(nodeHandle);
  err = GetDetailInfo (lpVirtNode, objType, lpMem, FALSE, &SesHndl);

  // verification that there are no UDTs like column type.
  switch (objType) {
    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
      lpObj = ((LPTABLEPARAMS)(lpMem))->lpColumns;
      while(lpObj)
      {
          LPCOLUMNPARAMS lpCol = (LPCOLUMNPARAMS)lpObj->lpObject;
          if (lstrcmpi(lpCol->tchszInternalDataType, lpCol->tchszDataType) != 0)  {
              assert (lpCol->tchszInternalDataType[0]);
              //"Table '%s' has a '%s' column (Spatial or User-defined data type).\n"
              //"The Drag And Drop functionality is not available for this table."
              wsprintf(szTxt, ResourceString ((UINT)IDS_E_TABLE_WITH_UDT_COLUMN_TYPE),
                              ((LPTABLEPARAMS)(lpMem))->objectname,
                              lpCol->tchszInternalDataType);
              TS_MessageBox(TS_GetFocus(),(LPCTSTR) szTxt,NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
              err = RES_ERR;
              break;
          }
          if (lpCol->uDataType == SQL_UNKOWN_TYPE)
          {
              //"Table '%s' has a '%s' column.\n"
              //"The Drag And Drop functionality is not available for this table."
              wsprintf(szTxt,ResourceString ((UINT)IDS_E_TABLE_WITH_UNKNOWN_COLUMN_TYPE),
                              ((LPTABLEPARAMS)(lpMem))->objectname,
                              lpCol->tchszDataType);
              TS_MessageBox(TS_GetFocus(),(LPCTSTR) szTxt,NULL, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
              err = RES_ERR;
              break;
          }
          lpObj = lpObj->lpNext;
      }
      break;
  }

  return err;
}

//
//  Clears the DOM object in the clipboard, if any
//  (clear feature for Attached Pointers free management)
//
void  ClearDomObjectFromClipboard(HWND hwnd)
{
  PasteDomObjectFromClipboard(hwnd, NULL, TRUE);
}

//
//  pastes the object characteristics from the clipboard,
//  or clears the clipboard if last parameter is TRUE
//  (clear feature for Attached Pointers free management)
//
// returns TRUE if successful
//
BOOL PasteDomObjectFromClipboard(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bClear)
{
  UINT        size;
  HANDLE      hClip;
  CHAR       *lpClip;
  void       *lpMem;
  int         objType;
  int         res;

  if (lpDomData)
    objType = DomClipboardObjType;
  else {
    if (DomClipboardLastStoredType == 0)
      return FALSE;       // Nothing to paste
    objType = DomClipboardLastStoredType;
  }

  // Pre-test on compatibility between the clipboard-saved object type
  // and the current selection in the destination tree
  // TO BE FINISHED

  if (!OpenClipboard(hwndMdi)) {
    if (!bClear)
      ErrorClipboardPasteDomObj(0, TRUE);
    return FALSE;
  }

  hClip = GetClipboardData (wDomClipboardFormat);
  if (!hClip) {
    if (!bClear)
      ErrorClipboardPasteDomObj(0, TRUE);
    return FALSE;
  }
  lpClip = GlobalLock(hClip);
  if (!lpClip) {
    if (!bClear)
      ErrorClipboardPasteDomObj(0, TRUE);
    return FALSE;
  }

  // ask for necessary size, and allocate as requested
  size = GetDetailsSize(objType);

  if (!size) {
    GlobalUnlock(hClip);
    CloseClipboard();
    if (!bClear)
      ErrorClipboardPasteDomObj(0, TRUE);
    return FALSE;
  }
  lpMem = ESL_AllocMem(size);
  if (!lpMem) {
    GlobalUnlock(hClip);
    CloseClipboard();
    if (!bClear)
      ErrorClipboardPasteDomObj(0, TRUE);
    return FALSE;
  }

  // copy into our buffer
  memcpy(lpMem, lpClip, size);
  GlobalUnlock(hClip);

  if (bClear) {
    FreeAttachedPointers(lpMem, objType);
    EmptyClipboard();
  }

  // make the clipboard freewheeling
  CloseClipboard();

  // Create the object, or reset DomClipboardLastStoredType
  if (bClear)
    DomClipboardLastStoredType = 0;
  else {
    res = CreateObjFromClipboardData(hwndMdi, lpDomData, lpMem, size,
                                     objType);
    if (res != RES_SUCCESS) {
      ESL_FreeMem(lpMem);
      ErrorClipboardPasteDomObj(res, FALSE);
      return FALSE;
    }
  }

  // free the allocated data
  ESL_FreeMem(lpMem);

  return TRUE;
}

//
//  creates the object starting from the characteristics
//  read from the clipboard
//
// returns TRUE if successful
//
static int NEAR CreateObjFromClipboardData(HWND hwndMdi, LPDOMDATA lpDomData, void *lpMem, int size, int objType)
{
  LPTREERECORD    lpRecord;
  DWORD           dwCurSel;
  int             ires;
  LPUCHAR         lpVirtNode;
  int             level;
  int             iAnswer;
  LPUCHAR         aparents[MAXPLEVEL];      // new parents
  UCHAR           bufPar0[MAXOBJECTNAME];   // parent string of level 0
  UCHAR           bufPar1[MAXOBJECTNAME];   // parent string of level 1
  UCHAR           bufPar2[MAXOBJECTNAME];   // parent string of level 2
  UCHAR           szTxt[2048];              // TO BE FINISHED should be loaded using LoadString
  UCHAR           szFromDb[MAXOBJECTNAME];  // From database (copy)
  UCHAR           szToOwner[MAXOBJECTNAME]; // New owner (Paste time)
  UCHAR           szFromOwner[MAXOBJECTNAME]; // Owner at copy time
  GROUPUSERPARAMS groupUserParams;          // special for groupuser paste

  BOOL bInterrupted;
  UINT uiIntType;

  // anchor aparentsResult on buffers
  aparents[0] = bufPar0;
  aparents[1] = bufPar1;
  aparents[2] = bufPar2;
  bufPar0[0] = bufPar1[0] = bufPar2[0] = '\0';

  // Get current record information, for parenthood update
  dwCurSel = GetCurSel(lpDomData);
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                          LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  x_strcpy(aparents[0], lpRecord->extra);
  x_strcpy(aparents[1], lpRecord->extra2);
  x_strcpy(aparents[2], lpRecord->extra3);

  // Special management: paste a user in a group, i.e. make a groupuser
  if (objType==OT_USER) {
    if (lpRecord->recType == OT_GROUPUSER ||
        lpRecord->recType == OT_STATIC_GROUPUSER) {
      objType = OT_GROUPUSER;
      x_strcpy(groupUserParams.ObjectName,
             ((LPUSERPARAMS)(lpMem))->ObjectName);
      lpMem = (LPGROUPUSERPARAMS)&groupUserParams;
    }
  }

  // calculate level from object type, for calls to UpdateDomData
  level = GetParentLevelFromObjType(objType);

  // update the parenthood, according to object type
  switch (objType) {
    case OT_GROUP:
      // No parenthood
      break;

    case OT_INTEGRITY:
      // update parent base name
      x_strcpy(((LPINTEGRITYPARAMS)(lpMem))->DBName, aparents[0]);
      // update parent table name
      x_strcpy(((LPINTEGRITYPARAMS)(lpMem))->TableName, aparents[1]);
      break;

    case OT_LOCATION:
      // No parenthood
      break;

    case OT_PROCEDURE:
    case OT_SCHEMAUSER_PROCEDURE:
      // update parent base name
      x_strcpy(szFromOwner,((LPPROCEDUREPARAMS)(lpMem))->objectowner);
      x_strcpy(((LPPROCEDUREPARAMS)(lpMem))->DBName, aparents[0]);
      break;

    case OT_SEQUENCE:
      // update parent base name
      x_strcpy(szFromOwner,((LPSEQUENCEPARAMS)(lpMem))->objectowner);
      x_strcpy(((LPSEQUENCEPARAMS)(lpMem))->DBName, aparents[0]);
      break;

    case OT_RULE:
      // update parent base name
      x_strcpy(szFromOwner,((LPRULEPARAMS)(lpMem))->RuleOwner);
      x_strcpy(((LPRULEPARAMS)(lpMem))->DBName, aparents[0]);
      // update parent table name
      x_strcpy(((LPRULEPARAMS)(lpMem))->TableName, aparents[1]);
      break;

    case OT_USER:
      // No parenthood
      break;

    case OT_VIEW:
    case OT_SCHEMAUSER_VIEW:
      // update parent base name
      x_strcpy(((LPVIEWPARAMS)(lpMem))->DBName, aparents[0]);
      // don't forget the owner of the view
      x_strcpy(szFromOwner,((LPVIEWPARAMS)(lpMem))->szSchema);
      break;

    case OT_DBEVENT:
      // update parent base name
      x_strcpy(((LPDBEVENTPARAMS)(lpMem))->DBName, aparents[0]);
      break;

    case OT_TABLE:
    case OT_SCHEMAUSER_TABLE:
        x_strcpy(szFromOwner,((LPTABLEPARAMS)(lpMem))->szSchema);
        x_strcpy(szFromDb,((LPTABLEPARAMS)(lpMem))->DBName);
        x_strcpy(((LPTABLEPARAMS)(lpMem))->DBName, aparents[0]);
        // Emb Dec.16, 96: remove page size specification for 1.2 node
        //if (GetOIVers() == OIVERS_12)
        if (lpDomData->ingresVer == OIVERS_12) {
          // Emb Dec. 1, 97: test page size against 2K
          LONG  pgSize = ((LPTABLEPARAMS)(lpMem))->uPage_size ;
          if ( pgSize != 0 && pgSize != 2048) {
            char buf[BUFSIZE];
            //"Page Size will be Decreased from %ld to 2K.\n"
            //"Table Create may fail if Row Width is greater than 2K"
            wsprintf(buf,ResourceString(IDS_F_PAGE_SIZE) ,((LPTABLEPARAMS)(lpMem))->uPage_size );
            MessageBox(GetFocus(),buf,
                       ResourceString(IDS_TITLE_WARNING),
                       MB_ICONEXCLAMATION | MB_OK);
            ((LPTABLEPARAMS)(lpMem))->uPage_size = 0;
          }
        }
      break;

    case OT_INDEX:
      {
        // update parent base name
        x_strcpy(szFromOwner,((LPINDEXPARAMS)(lpMem))->objectowner);
        x_strcpy(((LPINDEXPARAMS)(lpMem))->DBName, aparents[0]);
        // update parent table name
        x_strcpy(((LPINDEXPARAMS)(lpMem))->TableName, RemoveDisplayQuotesIfAny(StringWithoutOwner(aparents[1])));
		x_strcpy(((LPINDEXPARAMS)(lpMem))->TableOwner,lpRecord->tableOwner);
      }
      break;

    case OT_ROLE:
      // No parenthood
      break;

    case OT_GROUPUSER:
      // update parent group name
      x_strcpy(((LPGROUPUSERPARAMS)(lpMem))->GroupName, aparents[0]);
      break;

    default:
      return RES_ERR;
  }

  // check schema if concerned with
  // (table, index, procedure and rule)
  // cannot use CanObjectHaveSchema because of DBEVENT
  lpVirtNode = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
  if (objType==OT_TABLE || objType==OT_INDEX
                        || objType==OT_VIEW
                        || objType==OT_PROCEDURE
                        || objType==OT_RULE) {
    BOOL bGetUser = FALSE;
    int  oldIngresVer = SetOIVers(lpDomData->ingresVer);
	bGetUser = DBAGetSessionUser(lpVirtNode, szToOwner);
	SetOIVers(oldIngresVer);
    if (bGetUser) {
      if (x_strcmp(szToOwner, szFromOwner)!=0) {

        // Warning : The Schema will change from %s to %s\n\n"
        // Do you want to create the object in the new schema(%s) ?\n"
        // YES : Create into the new schema.\n"
        // NO  : Do not create",

        wsprintf(szTxt,
           ResourceString ((UINT)IDS_W_SCHEMA_WILL_CHANGE),
           szFromOwner,
           szToOwner,
           szToOwner);
        iAnswer=TS_MessageBox(TS_GetFocus(), szTxt, ResourceString ((UINT)IDS_I_PASTE_OBJECT), // Paste Object
                         MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL);
        if (iAnswer==IDNO)
            return RES_SUCCESS;
        switch (objType) {
          case OT_TABLE:
            x_strcpy (((LPTABLEPARAMS)(lpMem))->szSchema, szToOwner);
            x_strcpy (((LPTABLEPARAMS)(lpMem))->StorageParams.objectowner, szToOwner);
            break;
          case OT_INDEX:
            x_strcpy (((LPINDEXPARAMS)(lpMem))->objectowner, szToOwner);
            break;
          case OT_VIEW:
            x_strcpy (((LPVIEWPARAMS)(lpMem))->szSchema, szToOwner);
            break;
          case OT_PROCEDURE:
            x_strcpy (((LPPROCEDUREPARAMS)(lpMem))->objectowner, szToOwner);
            break;
          case OT_RULE:
            x_strcpy (((LPRULEPARAMS)(lpMem))->RuleOwner, szToOwner);
            break;
        }
      }
    }
    else {    // of if (DBAGetSessionUser(lpVirtNode, szToOwner)) {
      // not an error if destination is a gateway
      if (lpDomData->ingresVer != OIVERS_NOTOI) {
        TS_MessageBox(TS_GetFocus(),
                   ResourceString(IDS_DRAGANDDROP_DEST_ERR),
                   NULL,
                   MB_ICONEXCLAMATION | MB_OK);
        return RES_ERR;
      }
    }
  }

  {
    // Standard drag-drop case : Add the object
    int  oldIngresVer = SetOIVers(lpDomData->ingresVer);
    if  (objType == OT_INDEX)
    {
        DMLCREATESTRUCT cr;
        memset (&cr, 0, sizeof(cr));
        INDEXPARAMS2DMLCREATESTRUCT (lpMem, &cr);
        ires = DBAAddObject (lpVirtNode, objType, &cr);
    }
    else
        ires = DBAAddObject (lpVirtNode, objType, lpMem);
    SetOIVers(oldIngresVer);
    if (ires != RES_SUCCESS)
      return ires;
  }

  if (HasLoopInterruptBeenRequested())
    return RES_ERR;

  // If a table was added ask the user whether he wishes to copy the data
  // except for ingres to desktop operation
  if (objType==OT_TABLE) {
      // The table %s.%s has been successfully created.\n\n"
      // Do you wish to populate the new table with the"
      // original data",

      wsprintf(
            szTxt,
            ResourceString ((UINT)IDS_C_POPULATE_NEWTABLE),
            ((LPTABLEPARAMS)(lpMem))->szSchema,
            StringWithoutOwner(((LPTABLEPARAMS)(lpMem))->objectname));
      iAnswer = TS_MessageBox(TS_GetFocus(), szTxt, ResourceString ((UINT)IDS_I_PASTE_OBJECT),
                           MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL);

      if (iAnswer==IDYES) {
          LPTSTR lpCollist = NULL;
          if (((LPTABLEPARAMS)(lpMem))->bGWConvertDataTypes)
             lpCollist = GenerateColList( lpMem );

          ires=CopyDataAcrossTables (
             ((LPTABLEPARAMS)(lpMem))->szNodeName,  // From Node Name
             szFromDb,                              // data base at copy time
             szFromOwner,                           // table owner at copy time
             ((LPTABLEPARAMS)(lpMem))->objectname,  // From table name
             lpVirtNode,                            // To node name
             ((LPTABLEPARAMS)(lpMem))->DBName,      // To datatabase
             szToOwner,                             // Owner (paste)
             ((LPTABLEPARAMS)(lpMem))->objectname,  // To table name
             ((LPTABLEPARAMS)(lpMem))->bGWConvertDataTypes,
             lpCollist);
          if (lpCollist)
              ESL_FreeMem(lpCollist);

        if (ires != RES_SUCCESS) {
          if (ires==RES_ENDOFDATA)
             //
             // No Data was read - empty table created.
             //
             TS_MessageBox(TS_GetFocus(), ResourceString ((UINT)IDS_I_NODATA_READ),
                               ResourceString ((UINT)IDS_I_PASTE_OBJECT),
                               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
          else
             //
             // Populate unsuccessful - empty table created.
             //
             TS_MessageBox(TS_GetFocus(), ResourceString ((UINT)IDS_E_POPULATE_UNSUCCESS),
                               ResourceString ((UINT)IDS_I_PASTE_OBJECT),
                               MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
          ires = RES_SUCCESS; // as if successful
        }
      }
  }    
  
//UpdateCacheAndTrees:

  // Update the cache and the tree(s)
  DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, 0);
  bInterrupted = HasLoopInterruptBeenRequested();
  if (bInterrupted) {
    uiIntType = VDBA_GetInterruptType();
    VDBA_SetInterruptType(INTERRUPT_NOT_ALLOWED);
  }

  // Must manage oivers in case destination doc is on a gateway
  {
    int  oldIngresVer = SetOIVers(lpDomData->ingresVer);
    UpdateDOMData(
       lpDomData->psDomNode->nodeHandle, // hnodestruct
       objType,                          // iobjecttype
       level,                            // level
       aparents,                         // parentstrings
       FALSE,                            // keep system tbls state
       FALSE);                           // bWithChildren
    if (bInterrupted)
    VDBA_SetInterruptType(uiIntType);

    DOMUpdateDisplayData (
       lpDomData->psDomNode->nodeHandle, // hnodestruct
       objType,                          // iobjecttype
       level,                            // level
       aparents,                         // parentstrings
       FALSE,                            // bWithChildren
       ACT_ADD_OBJECT,                   // object added
       0L,                               // no item id
       0);                               // all mdi windows concerned
    DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);
    SetOIVers(oldIngresVer);
  }

  return RES_SUCCESS;
}

//--------------------------------------------------------------------------
//
// Tree multiple branches expansion functions
//
//--------------------------------------------------------------------------

int ManageExpandBranch(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  return Task_ManageExpandBranchInterruptible(hwndMdi, lpDomData, dwItem, hWndTaskDlg);
}
  
int ManageExpandBranch_MT(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  DWORD dwCurItem;
  int   objType;
  int   childType;
  DWORD dwFirstItem;
  DWORD dwNextItem;
  DWORD dwParentLevel;

  if (HasLoopInterruptBeenRequested())
    return RES_ERR;

  // recursively expand until xref met
  if (dwItem==0) {
    dwCurItem  = GetCurSel(lpDomData);
    objType = GetCurSelObjType(lpDomData);
  }
  else {
    dwCurItem = dwItem;
    objType = GetItemObjType(lpDomData, dwCurItem);
  }

  // which object would be created?
  childType = GetChildType(objType, FALSE);

  // stop if xref
  if (IsRelatedType(childType))
    return RES_SUCCESS;

  // stop if only sub-statics leading to xref
  if (childType == OT_STATIC_DUMMY) {
    switch (objType)
      case OT_STATIC_R_GRANT:
      case OT_STATIC_R_SECURITY:
      return RES_SUCCESS;

  }

  //
  // remainder : valid data type (including OT_GROUPUSER, OT_TABLELOCATION,
  //           OT_VIEWTABLE, OT_RULEPROC, OT_SCHEMAUSER, OT_SYNONYMOBJECT,
  //           and all GRANTS)
  //

  // Update notification message if animated progress dialog box
  if (hWndTaskDlg) {
    LPTREERECORD lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                       LM_GETITEMDATA, 0, (LPARAM)dwCurItem);
    char buf[MAXOBJECTNAME * 4 + MAXOBJECTNAME];
    objType = GetItemObjType(lpDomData, dwCurItem);
    if (IsItemObjStatic(lpDomData, dwCurItem)) {
      int childType = GetChildType(objType, TRUE);    // No solve
      int level = GetParentLevelFromObjType(childType);
      switch (level) {
        case 0:
          wsprintf (buf, "%s", lpRecord->objName);
          break;
        case 1:
        // tricky for installation level settings: level is 1 but no parent database
          if (lpRecord->extra[0])
            wsprintf (buf, "%s of [%s]", lpRecord->objName, lpRecord->extra);
          else
            wsprintf (buf, "%s", lpRecord->objName);
          break;
        case 2:
          wsprintf (buf, "%s of [%s][%s]", lpRecord->objName, lpRecord->extra, lpRecord->extra2);
          break;
        case 3:
          wsprintf (buf, "%s of [%s][%s][%s]", lpRecord->objName, lpRecord->extra, lpRecord->extra2, lpRecord->extra3);
          break;
        default:
          assert(FALSE);
          break;
      }
      NotifyAnimationDlgTxt(hWndTaskDlg, (LPCTSTR)buf);
    }
  }

  // Expand the branch - synchronous notification msg needed!
  SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0xabcd, (LPARAM)dwCurItem);

  // loop on the brand new children (GetNextImmediateChild reentrant)
  dwFirstItem = GetFirstImmediateChild(lpDomData, dwCurItem, &dwParentLevel);
  dwNextItem = dwFirstItem;
  while (dwNextItem) {
    int iret = ManageExpandBranch(hwndMdi, lpDomData, dwNextItem, hWndTaskDlg);
    if (iret != RES_SUCCESS)
      return iret;
    if (HasLoopInterruptBeenRequested())
      return RES_ERR;
    dwNextItem = GetNextImmediateChild(lpDomData, dwCurItem, dwNextItem,
                                       dwParentLevel);
  }
  return RES_SUCCESS;
}

int ManageExpandAll(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  return Task_ManageExpandAllInterruptible(hwndMdi, lpDomData, hWndTaskDlg);
}
  
int ManageExpandAll_MT(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  DWORD dwFirstItem;
  DWORD dwNextItem;

  // loop on the level 0 branches
  dwFirstItem = GetFirstLevel0Item(lpDomData);
  dwNextItem = dwFirstItem;
  while (dwNextItem) {
    int res = ManageExpandBranch(hwndMdi, lpDomData, dwNextItem, hWndTaskDlg);
    if (res != RES_SUCCESS)
      return res;
    dwNextItem = GetNextLevel0Item(lpDomData, dwNextItem);
  }
  return RES_SUCCESS;
}

#define FORCEREFRESH_MAXRECURS  60
static int depthLevel;

//
// Manage the "force refresh branch and sub-branches" :
// for each sub-branch, calls UpdateDomDataDetail()
// with parameters set so that multiple updates of the same list
// of objects will only make one request on the server.
//
// CAUTION : PrepareGlobalDOMUpd MUST have been called first
//
// After this function has been called, the displays of all doms
// on the same node MUST be updated by calling UpdateDisplayData()
// with OT_VIRTNODE
//
// See full sequence in domdisp.c, search for  "case FORCE_REFRESH_CURSUB:"
//

int ManageForceRefreshCursub(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  return Task_ManageForceRefreshCursubInterruptible(hwndMdi, lpDomData, dwItem, hWndTaskDlg);
}

int ManageForceRefreshCursub_MT(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg)
{
  DWORD         dwCurItem;
  int           objType;
  DWORD         dwFirstItem;
  DWORD         dwNextItem;
  LPTREERECORD  lpRecord;
  int           iret;
  int           level;
  LPUCHAR       aparents[MAXPLEVEL];      // new parents
  DWORD         dwParentLevel;

  // recursively scan the branch until "not expanded yet"
  // or "dummy item" or terminal branch

  if (HasLoopInterruptBeenRequested())
    return RES_ERR;

  if (dwItem==0) {
    dwCurItem  = GetCurSel(lpDomData);
    objType = GetCurSelObjType(lpDomData);
    depthLevel = 0;
  }
  else {
    dwCurItem = dwItem;
    objType = GetItemObjType(lpDomData, dwCurItem);
  }

  // check maximum recursivity calls not reached
  if (depthLevel >= FORCEREFRESH_MAXRECURS)
    return RES_ERR;

  // is this a dummy branch ?
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurItem);
  if (lpRecord==0 || IsNoItem(objType, lpRecord->objName))
    return RES_SUCCESS;

  // if the object is static and has subdata valid (has been expanded),
  // update the cache with the corresponding non-static type
  // Example: OT_STATIC_DATABASE -> OT_DATABASE
  if (lpRecord->bSubValid) {
    objType = GetChildType(objType, TRUE);  // No solve for UpdateDOMDataDetail
    if (objType != OT_STATIC_DUMMY) {
      level = GetParentLevelFromObjType(objType);
      aparents[0] = lpRecord->extra;
      aparents[1] = lpRecord->extra2;
      aparents[2] = lpRecord->extra3;

      // Update notification message if animated progress dialog box
      if (hWndTaskDlg) {
        char buf[MAXOBJECTNAME * 4 + MAXOBJECTNAME];
        switch (level) {
          case 0:
            wsprintf (buf, "%s", lpRecord->objName);
            break;
          case 1:
          // tricky for installation level settings: level is 1 but no parent database
            if (lpRecord->extra[0])
              wsprintf (buf, "%s %s [%s]", lpRecord->objName, ResourceString((UINT) IDS_OF), lpRecord->extra);
            else
              wsprintf (buf, "%s", lpRecord->objName);
            break;
          case 2:
            wsprintf (buf, "%s %s [%s][%s]", lpRecord->objName, ResourceString((UINT) IDS_OF),lpRecord->extra, lpRecord->extra2);
            break;
          case 3:
            wsprintf (buf, "%s %s [%s][%s][%s]", lpRecord->objName, ResourceString((UINT) IDS_OF),lpRecord->extra, lpRecord->extra2, lpRecord->extra3);
            break;
          default:
            assert(FALSE);
            break;
        }
        NotifyAnimationDlgTxt(hWndTaskDlg, (LPCTSTR)buf);
      }

      // Update low level
      iret = UpdateDOMDataDetail(lpDomData->psDomNode->nodeHandle,
                                objType,
                                level,
                                aparents,                // aparents
                                lpDomData->bwithsystem,
                                TRUE,                    // bWithChildren
                                TRUE,                    // Only If Exists
                                TRUE,                    // bUnique
                                FALSE);                  // bWithDisplay
      if (iret != RES_SUCCESS)
        return RES_ERR;                 // ABNORMAL ! Don't go depper in this branch

      if (HasLoopInterruptBeenRequested())
        return RES_ERR;

      if (objType == OT_DBEVENT)
        UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle,aparents[0]);

    }

    // loop on the children of the branch
    dwFirstItem = GetFirstImmediateChild(lpDomData, dwCurItem, &dwParentLevel);
    dwNextItem = dwFirstItem;
    while (dwNextItem) {
      int iret;
      depthLevel++;
      iret = ManageForceRefreshCursub(hwndMdi, lpDomData, dwNextItem, hWndTaskDlg);
      depthLevel--;
      if (iret != RES_SUCCESS)
        return iret;
      dwNextItem = GetNextImmediateChild(lpDomData, dwCurItem, dwNextItem,
                                        dwParentLevel);
    }
  }

  return RES_SUCCESS;
}

//
// Manage the "force refresh all branches" :
// for each sub-branch, calls UpdateDomDataDetail()
// with parameters set so that multiple updates of the same list
// of objects will only make one request on the server.
//
// CAUTION : PrepareGlobalDOMUpd MUST have been called first
//
// After this function has been called, the displays of all doms
// on the same node MUST be updated by calling UpdateDisplayData()
// with OT_VIRTNODE
//
// See full sequence in domdisp.c, search for
//   "case FORCE_REFRESH_ALLBRANCHES:"
//

int ManageForceRefreshAll(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  return Task_ManageForceRefreshAllInterruptible(hwndMdi, lpDomData, hWndTaskDlg);
}

int ManageForceRefreshAll_MT(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg)
{
  DWORD dwFirstItem;
  DWORD dwNextItem;

  // loop on the level 0 branches
  dwFirstItem = GetFirstLevel0Item(lpDomData);
  dwNextItem = dwFirstItem;
  while (dwNextItem) {
    ManageForceRefreshCursub(hwndMdi, lpDomData, dwNextItem, hWndTaskDlg);

    if (HasLoopInterruptBeenRequested())
      return RES_ERR;

    dwNextItem = GetNextLevel0Item(lpDomData, dwNextItem);
  }

  return RES_SUCCESS;
}

