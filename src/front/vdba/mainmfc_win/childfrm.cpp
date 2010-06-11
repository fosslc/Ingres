/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : childfrm.cpp : implementation of the CChildFrame class 
**    Project  : IngresII / VDBA.
**    Author   : Emmanuel Blattes
**
**  History after 01-01-2000
**   14-Jan-2000 (noifr01)
**     Bug #100015
**      -detect whether the expiration date of a table was stored with the
**       same timezone as the current one. If not, provide a warning
**       because of the granularity of the expiration date dialog, which 
**       reflects that of the "save ... until" SQL query (1 day granularity)
**      -updated the date formatting function, in order to take into account
**       the new format of the initial string: 4 digits year (no more need to
**       add the century here), and time at the end  of the string
**  26-May-2000 (noifr01)
**     (bug 99242) cleanup for DBCS compliance
**  26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**  16-Nov-2000 (schph01)
**      sir 102821 Comment on table and columns.
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
**  11-Apr-2001 (schph01)
**    (sir 103292) add menu for 'usermod' command.
**  27-Apr-2001 (schph01)
**    (sir 103299) new menu management for 'copydb' command.
**                 - II 2.6 version and more the new dialog box is launched,
**                   otherwise no change: the current dialog is launched
**  08-Aug-2002 (hanje04 for noifr01)
**	BUG 108470
**	To stop VDBA running out of handles when expanding many, many branches
**	(e.g when doing an expand all with system objects), make 
**	TS_DOMRightPaneUpdateBitmap just return null. The bitmap on the branch
**	only needs to be updated when the branch is selected.
**  27-Mar-2003 (noifr01)
**    (bug 109837) disable functionality against older versions of Ingres,
**    that would require to update rmcmd on these platforms
**  28-Mar-2003 (noifr01)
**   (sir 107523) management of sequences
**  16-Sep-2003 (schph01)
**    (bug 110921) Enable the "Database"/"Infodb" menu, when connected to
**    remote node.
**  03-Feb-2004 (uk$so01)
**     SIR #106648, Split vdba into the small component ActiveX or COM:
**     Integrate Export assistant into vdba
**  10-Mar-2010 (drivi01) SIR 123397
**     Refresh the whole tree after rollforward completes, only if 
**     rollforward is executed from the main menu.
**  10-May-2010 (drivi01)
**     Enable\Disable file menus for Ingres VectorWise tables
**     depending on weather the menus apply.
**     Checkpoint/rollforward will be disabled for the entire
**     database in VectorWise installation due to the fact
**     that running backups/restores for the whole database
**     that contains Ingres VectorWise tables could result
**     in inconsistent catalog data.
**  25-May-2010 (drivi01) Bug 123817
**    Expand tampoo buffer to double the MAXOBJECTNAME
**    to account for delimeted ids such as username.table_name.
**  02-Jun-2010 (drivi01)
**    Remove hard coded buffer sizes.
**/

#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"

#include "childfrm.h"
#include "domfast.h"

#include "mainvi2.h"  // CMainMfcView2 - GetDomItemPageInfo()
#include "maindoc.h"
#include "mainview.h" // CMainMfcView

#include "fastload.h"
#include "duplicdb.h"
#include "starutil.h"   // IsLocalNodeName
#include "extccall.h"   // VDBA_EnableDisableSecurityLevel()
#include "dgexpire.h"   // dialog box for expiration date
#include "dgerrh.h"     // MessageWithHistoryButton
#include "extccall.h"
#include "cmdline.h"
#include "qsplittr.h"
#include "dgtbjnl.h"   // dialog box for table journaling
#include "tcomment.h"  // dialog box for object Comment
#include "usermod.h"   // dialog box for usermod command
#include "xcopydb.h"   // dialog box for copydb command
#include "infodb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "dba.h"
#include "dbaginfo.h"

#include "main.h"
#include "dom.h"

#include "treelb.e"   // LM_GETLEVEL, ...

LPDOMDATA GetLPDomData(CWnd* pWnd);
// from dom.c
BOOL DomOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void DomOnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
//void DomOnSysCommand(HWND hwnd, UINT cmd, int x, int y);
void DomOnMdiActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);
BOOL DomIsSystemChecked(HWND hWnd);

// from domsplit.c
BOOL HasTrueParentDB(LPDOMDATA lpDomData, char *dbName);
BOOL CanRegisterAsLink( int hnodestruct, LPTREERECORD lpRecord);
BOOL CanRefreshLink( int hnodestruct, LPTREERECORD lpRecord);

#include "tree.h"
#include "domdisp.h"
#include "resource.h"
};


//
//  Utilities
//
// Utilities - called from mainlib

extern "C" DWORD GetDomDocHelpId(HWND hWnd)
{
  CWnd* pWnd;
  pWnd = CWnd::FromHandlePermanent(hWnd);
  ASSERT (pWnd);
  if (!pWnd)
    return HELPID_MONITOR;  // default help screen
  CChildFrame* pFrm = (CChildFrame *)pWnd;
  return pFrm->GetHelpID();
}

static void AdjustRectForToolbar(CWnd* pWndToolbar, LPCTSTR lpText, CRect& rRect)
{
  ASSERT (pWndToolbar);
  ASSERT (rRect.left < 0);
  ASSERT (rRect.top < 0);
  ASSERT (rRect.right == 0);
  ASSERT (rRect.bottom == 0);

  // Select the toolbar font
  CClientDC dc(pWndToolbar);
  CFont* pFont = pWndToolbar->GetFont();
  CFont* oldFont = dc.SelectObject(pFont);

  // Calculate width, adding 5 spaces
  CString csTxt(lpText);
  csTxt += CString(_T(' '), 5);
  CSize refSize;
  refSize = dc.GetTextExtent(csTxt);

  // select the old font
  dc.SelectObject(oldFont);

  // adjust in width, taking control minimum size in account
  refSize.cx += GetSystemMetrics(SM_CXMENUCHECK);
  if (refSize.cy < GetSystemMetrics(SM_CYMENUCHECK))
    refSize.cy = GetSystemMetrics(SM_CYMENUCHECK);

  // Adjust received rectangle
  if (refSize.cx > -rRect.left)
    rRect.left = -refSize.cx;
  if (refSize.cy > -rRect.top)
    rRect.top = -refSize.cy;
}

static CString MakeFullNameWithParents(LPDOMDATA lpDomData, LPTREERECORD lpRecord, int CurItemObjType)
{
  ASSERT (lpRecord);

  CString csParents(_T(""));
  CString csObject(_T("Unknown"));

  if (IsCurSelObjStatic(lpDomData)) {
    // shift parenthood and use last parent as obj name
    if (lpRecord->extra3[0] != '\0') {
       csParents.Format(_T("[%s][%s]"), lpRecord->extra, lpRecord->extra2);
       csObject.Format (_T("%s"), lpRecord->extra3);
    }
    else if (lpRecord->extra2[0] != '\0') {
      csParents.Format(_T("(%s)"), lpRecord->extra);
      csObject.Format (_T("%s"), lpRecord->extra2);
    }
    else {
      csParents = _T("");
      csObject.Format (_T("%s"), lpRecord->extra);
    }
  }
  else {
    // parenthood
    if (CurItemObjType == OT_REPLICATOR) {
      csParents.Format(_T("(%s)"), lpRecord->extra);
    }
    else {
      if (lpRecord->extra3[0] != '\0')
        csParents.Format(_T("(%s)(%s)(%s)"), lpRecord->extra,
                                             lpRecord->extra2,
                                             lpRecord->extra3);
      else if (lpRecord->extra2[0] != '\0')
         csParents.Format(_T("(%s)(%s)"), lpRecord->extra, lpRecord->extra2);
      else if (lpRecord->extra[0] != '\0')
        csParents.Format(_T("(%s)"), lpRecord->extra);
      else
        csParents = _T("");
    }
    
    // object name plus schema if applicable
    if (CurItemObjType == OT_REPLICATOR)
      csObject = _T("");
    else {
      if (lpRecord->objName[0]) {
        if (HasOwner(CurItemObjType)) {
          // objName may contain schema.objname
          UCHAR tampoo[MAXOBJECTNAME*2];
          StringWithOwner(lpRecord->objName, lpRecord->ownerName, tampoo);
          csObject = tampoo;
        }
        else
          csObject.Format (_T("%s"), lpRecord->objName);
      }
      else
        csObject = _T("");    // < No xyz > - in case...
    }
  }

  // build result string
  CString csResult = csParents + _T(" ") + csObject;
  return csResult;
}

//
// used in dom.c
//
extern "C" HWND GetBaseOwnerComboHandle(HWND hWnd)
{
  HWND hwndFrame = GetParent(GetParent(hWnd));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    return (pChildFrame->GetComboOwner()).m_hWnd;
  }
  return 0;
}

extern "C" HWND GetOtherOwnerComboHandle(HWND hWnd)
{
  HWND hwndFrame = GetParent(GetParent(hWnd));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    return (pChildFrame->GetComboOwner2()).m_hWnd;
  }
  return 0;
}

extern "C" HWND GetSystemCheckboxHandle(HWND hWnd)
{
  HWND hwndFrame = GetParent(GetParent(hWnd));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    return (pChildFrame->GetCheckBoxSysObj()).m_hWnd;
  }
  return 0;
}


//
// used in tree.c
//

extern "C" void UpdateGlobalStatusForDom(LPDOMDATA lpDomData, LPARAM lParam, LPTREERECORD lpRecord)
{
  // Pick status bar pointer - see MainFrmUpdateStatusBarPane0() in mainmfr.cpp
  CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
  CStatusBar *pStatusBar = pMainFrame->GetMainFrmStatusBar();
  ASSERT (pStatusBar);
  if (!pStatusBar)
    return;

  // null lParam : not even default string needed
  if (!lParam)
    return;

  ASSERT (lpRecord);
  if (lpRecord) {
    if (IsItemObjStatic(lpDomData, (DWORD)lParam) || lpRecord->recType == OT_STATIC_INSTALL_SECURITY ) {
      // object type
      CString csType("Unknown");
      csType.LoadString(GetStaticStatusStringId(lpRecord->recType));

      // 2) parenthoods
      CString csParents("");
      if (lpRecord->extra3[0] != '\0')
        csParents.Format("[%s][%s][%s]", lpRecord->extra,
                                         lpRecord->extra2,
                                         lpRecord->extra3);
      else if (lpRecord->extra2[0] != '\0')
         csParents.Format("[%s][%s]", lpRecord->extra, lpRecord->extra2);
      else if (lpRecord->extra[0] != '\0')
        csParents.Format("[%s]", lpRecord->extra);
      else
        csParents = "";

      // resulting string
      CString csFormattedText("");
      csFormattedText.Format("%s %s", (LPCTSTR)csType, (LPCTSTR)csParents);
      pStatusBar->SetPaneText(0, csFormattedText);
    }
    else {
      // object type
      CString csType("Unknown");
      csType.LoadString(GetObjectTypeStringId(lpRecord->recType));

      // 2) parenthoods
      CString csParents("");
      if (lpRecord->recType == OT_REPLICATOR) {
        csParents.Format("(%s)", lpRecord->extra);
      }
      else {
        if (lpRecord->extra3[0] != '\0')
          csParents.Format("(%s)(%s)(%s)", lpRecord->extra,
                                           lpRecord->extra2,
                                           lpRecord->extra3);
        else if (lpRecord->extra2[0] != '\0')
           csParents.Format("(%s)(%s)", lpRecord->extra, lpRecord->extra2);
        else if (lpRecord->extra[0] != '\0')
          csParents.Format("(%s)", lpRecord->extra);
        else
          csParents = "";
      }

      // 3) object name plus schema if applicable
      CString csObject("Unknown");
      if (lpRecord->recType == OT_REPLICATOR)
        csObject = "";
      else {
        if (lpRecord->objName[0]) {
          if (HasOwner(lpRecord->recType)) {
            // objName may contain schema.objname
            UCHAR tampoo[MAXOBJECTNAME*2];
            StringWithOwner(lpRecord->objName, lpRecord->ownerName, tampoo);
            csObject = tampoo;
          }
          else
            csObject.Format (_T("%s"), lpRecord->objName);
        }
        else
          csObject = "";    // < No xyz >
      }

      // System attribute
      CString csSystem("Unknown");
      if (IsSystemObject(GetBasicType(lpRecord->recType),
          lpRecord->objName, lpRecord->ownerName))
        csSystem.LoadString(IDS_TS_SYSTEMOBJ);
      else
        csSystem.LoadString(IDS_TS_NONSYSTEMOBJ);
      if (csSystem.GetLength()) {
        // Intermediate string MANDATORY for "csSystem.Format("(%s)", (LPCTSTR)csSystem);"
        CString cs2;
        cs2.Format("(%s)", (LPCTSTR)csSystem);
        csSystem = cs2;
      }

      // resulting string
      CString csFormattedText;
      csFormattedText.Format("%s : %s %s %s",
                             (LPCTSTR)csType, (LPCTSTR)csParents,
                             (LPCTSTR)csObject, (LPCTSTR)csSystem);
      pStatusBar->SetPaneText(0, csFormattedText);
    }
  }
}

static BOOL bBlock = FALSE;
extern "C" void MfcBlockUpdateRightPane() {ASSERT (!bBlock); bBlock = TRUE; }
extern "C" void MfcAuthorizeUpdateRightPane() {ASSERT (bBlock); bBlock = FALSE; }
BOOL IsUpdateRightPaneBlocked() { return bBlock; }
  
extern "C" void TS_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear)
{
  /* Implemented but not used at this time, always call MT_MfcUpdateRightPane() */
	if (0 /* CanBeInSecondaryThead() */ ) {
		UPDATERIGHTPANEPARAMS params;
    params.hwndDoc        = hwndDoc       ;
    params.lpDomData      = lpDomData     ;
    params.bCurSelStatic  = bCurSelStatic ;
    params.dwCurSel       = dwCurSel      ;
    params.CurItemObjType = CurItemObjType;
    params.lpRecord       = lpRecord      ;
    params.bHasProperties = bHasProperties;
    params.bUpdate        = bUpdate       ;
    params.initialSel     = initialSel    ;
    params.bClear         = bClear        ;
		/* return (int) */ Notify_MT_Action(ACTION_UPDATERIGHTPANE, (LPARAM) &params);
	}
	else
		MT_MfcUpdateRightPane(hwndDoc, lpDomData, bCurSelStatic, dwCurSel, CurItemObjType, lpRecord, bHasProperties, bUpdate, initialSel, bClear);
}

void MT_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear)
{
  if (IsUpdateRightPaneBlocked())
    return;

  if (bClear)
    bHasProperties = FALSE;

  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDoc);
  // Need NOT TO ASSERT due to multithreaded drag-drop
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);

  int hDomNode = lpDomData->psDomNode->nodeHandle;
  ASSERT (hDomNode != -1);

  CWaitCursor hourGlass;

  IPMUPDATEPARAMS ups;
  memset (&ups, 0, sizeof (ups));

  CuDlgDomTabCtrl* pTabDlg  = pMainMfcDoc->GetTabDialog();
  ASSERT (pTabDlg);

  if (!bHasProperties) {
    pTabDlg->DisplayPage (NULL, bUpdate);
    return;
  }

  try 
  {
    //
    // Notify the right pane that the selection has been changed
    // and we are about to display the new property.
    pTabDlg->NotifyPageSelChanging(bUpdate);

    CuDomPageInformation* pPageInfo = GetDomItemPageInfo(CurItemObjType, lpRecord, lpDomData->ingresVer, hDomNode);
    if (!pPageInfo) {
      pTabDlg->DisplayPage (NULL, bUpdate);
      return;
    }
    ASSERT_VALID (pPageInfo);
    CString strItemWithParents = MakeFullNameWithParents(lpDomData, lpRecord, CurItemObjType);
    ups.nType   = CurItemObjType;
    ups.pStruct = lpRecord;
    ups.pSFilter= pMainMfcDoc->GetLpFilter();
    pPageInfo->SetUpdateParam (&ups);
    pPageInfo->SetTitle(strItemWithParents, NULL);

    //
    //  Get the Image (Icon) of the current selected Tree Item:
    HICON hIcon = CChildFrame::DOMTreeGetCurrentIcon(lpDomData->hwndTreeLb);
    pPageInfo->SetImage  (hIcon);

    // if unicenter driven: preset initialSel according to requested caption if any
    if (IsUnicenterDriven()) {
      CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
      ASSERT (pDescriptor);
      ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
      if (pDescriptor->HasTabCaption()) {
        CString csTabCaption = pDescriptor->GetTabCaption();
        ASSERT (!csTabCaption.IsEmpty());
        if (pDescriptor->HasSpecifiedTabOnly()) {
          ASSERT (pPageInfo->GetNumberOfPage() == 1);
          ASSERT (initialSel == 0);
        }
        else {
          BOOL bFound = FALSE;
          int   nbpages = pPageInfo->GetNumberOfPage();
          for (int cpt=0; cpt < nbpages; cpt++) {
            UINT id = pPageInfo->GetTabID (cpt);
	          CString str;
	          str.LoadString (id);
            if (str.CompareNoCase(csTabCaption) == 0) {
              initialSel = cpt;
              bFound = TRUE;
              break;
            }
          }
          if (!bFound)
            AfxMessageBox(VDBA_MfcResourceString(IDS_E_TAB_NOT_FOUND), MB_OK | MB_ICONEXCLAMATION);//"Specified Tab not found - All Tabs displayed"
        }
      }
    }
    pTabDlg->DisplayPage (pPageInfo, bUpdate, initialSel);
  }
  catch(CMemoryException* e)
  {
      VDBA_OutOfMemoryMessage();
      e->Delete();
  }
  catch(CResourceException* e)
  {
      BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_DLG));//"Cannot load dialog box"
      e->Delete();
  }
  catch(...)
  {
      CString strMsg = VDBA_MfcResourceString(IDS_E_CONSTRUCT_PROPERTY);//_T("Cannot construct the property pane");
      BfxMessageBox (strMsg);
  }
}


extern "C" void NotifyDomFilterChanged(HWND hwndDoc, LPDOMDATA lpDomData, int iAction)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDoc);
  ASSERT (pWnd);
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);

  // update current filter structure
  LPSFILTER psFilter =  pMainMfcDoc->GetLpFilter();
  psFilter->bWithSystem = lpDomData->bwithsystem;
  x_strcpy (psFilter->lpBaseOwner, (LPCSTR)lpDomData->lpBaseOwner);
  x_strcpy (psFilter->lpOtherOwner, (LPCSTR)lpDomData->lpOtherOwner);

  FilterCause because;
  switch(iAction) {
    case ACT_CHANGE_SYSTEMOBJECTS:
      because = FILTER_DOM_SYSTEMOBJECTS;
      break;
    case ACT_CHANGE_BASE_FILTER:
      because = FILTER_DOM_BASEOWNER;
      break;
    case ACT_CHANGE_OTHER_FILTER:
      because = FILTER_DOM_OTHEROWNER;
      break;
  }

  pMainMfcDoc->UpdateAllViews(pView, (LPARAM)(int)because);
}

extern "C" void NotifyRightPaneForBkTaskListUpdate(HWND hwndDoc, LPDOMDATA lpDomData, int iobjecttype)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDoc);
  ASSERT (pWnd);
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);

  // update current filter structure
  LPSFILTER psFilter =  pMainMfcDoc->GetLpFilter();
  psFilter->UpdateType = iobjecttype;

  // Notify right pane only
  pMainMfcDoc->UpdateAllViews(pView, (LPARAM)FILTER_DOM_BKREFRESH);
}



void NotifyRightPaneForBkTaskDetailUpdate(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bOnLoad, time_t refreshtime)
{
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDoc);
  ASSERT (pWnd);
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);

  // update current filter structure
  LPSFILTER psFilter =  pMainMfcDoc->GetLpFilter();
  psFilter->bOnLoad = bOnLoad;
  psFilter->refreshtime = refreshtime;

  // Notify right pane only
  pMainMfcDoc->UpdateAllViews(pView, (LPARAM)FILTER_DOM_BKREFRESH_DETAIL);
}

extern "C" BOOL RefreshPropWindows(BOOL bOnLoad, time_t refreshtime)
{
	if (CanBeInSecondaryThead()) {
		UPDATEREFRESHPROPSPARAMS params;
		params.bOnLoad      = bOnLoad;
		params.refreshtime  = refreshtime;
		return (BOOL) Notify_MT_Action(ACTIONREFRESHPROPSWINDOW, (LPARAM) &params);// for being executed in the main thread
	}
	else
		return RefreshPropWindows_MT(bOnLoad, refreshtime); 
}
extern "C" BOOL RefreshPropWindows_MT(BOOL bOnLoad, time_t refreshtime)
{
  BOOL  bRefreshed = FALSE;
  HWND  hwndCurDoc;

  // get first document handle (with loop to skip the icon title windows)
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

  while (hwndCurDoc) {
    // test monitor on requested node
    if (QueryDocType(hwndCurDoc) == TYPEDOC_DOM) {
      HWND hwndMfcDoc = GetVdbaDocumentHandle(hwndCurDoc);
      LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndMfcDoc, 0);
      ASSERT (lpDomData);
      NotifyRightPaneForBkTaskDetailUpdate(hwndMfcDoc, lpDomData, bOnLoad, refreshtime);
      bRefreshed = TRUE;  // Act as if something has been refreshed
    }

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }

  return bRefreshed;
}


extern "C" void MfcClearReplicRightPane(HWND hwndDoc, LPTREERECORD lpRecord)
{
  ASSERT (!IsUpdateRightPaneBlocked());
  if (IsUpdateRightPaneBlocked())
    return;

  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndDoc);
  ASSERT (pWnd);
  if (!pWnd)
    return;
  CMainMfcView *pView = (CMainMfcView*)pWnd;
  CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
  ASSERT (pMainMfcDoc);

  CuDlgDomTabCtrl* pTabDlg  = pMainMfcDoc->GetTabDialog();
  ASSERT (pTabDlg);
  if (pTabDlg->GetCurrentProperty())
    ASSERT (pTabDlg->GetCurrentProperty()->GetDomPageInfo());   // needs to be reset along with deleted pPageInfo

  ASSERT (lpRecord);
  if (lpRecord) {
    if (lpRecord->bPageInfoCreated && lpRecord->m_pPageInfo) {
      // Right pane already loaded
      CuDomPageInformation* pPageInfo = (CuDomPageInformation*)lpRecord->m_pPageInfo;
      ASSERT_VALID(pPageInfo);

      // MANDATORY: reset current property dom page info if same as pPageInfo
      if (pTabDlg->GetCurrentProperty())
        if (pTabDlg->GetCurrentProperty()->GetDomPageInfo() == pPageInfo)
          pTabDlg->GetCurrentProperty()->SetDomPageInfo(NULL);

      // delete pPageInfo and reset associated flags
      delete pPageInfo;
      lpRecord->m_pPageInfo = NULL;
      lpRecord->bPageInfoCreated = FALSE;
    }
    else {
      // Right pane not loaded yet - simply check consistency between m_pPageInfo and bPageInfoCreated
      ASSERT (!lpRecord->bPageInfoCreated);
      ASSERT (!lpRecord->m_pPageInfo);
    }
  }
}

extern "C" BOOL MfcFastload(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonFastload();
  }
  return FALSE;
}

extern "C" BOOL MfcInfodb(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonInfodb();
  }
  return FALSE;
}
 
extern "C" BOOL MfcDuplicateDb(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonDuplicateDb();
  }
  return FALSE;
}
 
extern "C" BOOL MfcExpireDate(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonExpireDate();
  }
  return TRUE;    // can change the right pane!
}
 
extern "C" BOOL MfcComment(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonComment();
  }
  return TRUE;    // can change the right pane!
}

extern "C" BOOL MfcUsermod(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonUsermod();
  }

  return TRUE;    // can change the right pane!
}

extern "C" BOOL MfcCopyDB(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonCopyDB();
  }

  return TRUE;    // can change the right pane!
}

extern "C" BOOL MfcSecurityAudit(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonSecurityAudit();
  }
  return FALSE;
}
 

extern "C" BOOL MfcJournaling(HWND hwndDoc)
{
  HWND hwndFrame = GetParent(GetParent(hwndDoc));    // 2 levels since splitbar
  CWnd *pWnd;
  pWnd = CWnd::FromHandlePermanent(hwndFrame);
  ASSERT (pWnd);
  if (pWnd) {
    ASSERT (pWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    CChildFrame *pChildFrame = (CChildFrame *)pWnd;
    pChildFrame->RelayerOnButtonJournaling();
  }
  return TRUE;    // can change the right pane!
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_SPACECALC, OnUpdateButtonSpacecalc)
	ON_WM_CREATE()
	ON_WM_MDIACTIVATE()
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ADDOBJECT, OnUpdateButtonAddobject)
	ON_COMMAND(ID_BUTTON_ADDOBJECT, OnButtonAddobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ALTEROBJECT, OnUpdateButtonAlterobject)
	ON_COMMAND(ID_BUTTON_ALTEROBJECT, OnButtonAlterobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DROPOBJECT, OnUpdateButtonDropobject)
	ON_COMMAND(ID_BUTTON_DROPOBJECT, OnButtonDropobject)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PRINT, OnUpdateButtonPrint)
	ON_COMMAND(ID_BUTTON_PRINT, OnButtonPrint)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FIND, OnUpdateButtonFind)
	ON_COMMAND(ID_BUTTON_FIND, OnButtonFind)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_EXPANDONE, OnUpdateButtonClassbExpandone)
	ON_COMMAND(ID_BUTTON_CLASSB_EXPANDONE, OnButtonClassbExpandone)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_EXPANDBRANCH, OnUpdateButtonClassbExpandbranch)
	ON_COMMAND(ID_BUTTON_CLASSB_EXPANDBRANCH, OnButtonClassbExpandbranch)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_EXPANDALL, OnUpdateButtonClassbExpandall)
	ON_COMMAND(ID_BUTTON_CLASSB_EXPANDALL, OnButtonClassbExpandall)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_COLLAPSEONE, OnUpdateButtonClassbCollapseone)
	ON_COMMAND(ID_BUTTON_CLASSB_COLLAPSEONE, OnButtonClassbCollapseone)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_COLLAPSEBRANCH, OnUpdateButtonClassbCollapsebranch)
	ON_COMMAND(ID_BUTTON_CLASSB_COLLAPSEBRANCH, OnButtonClassbCollapsebranch)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CLASSB_COLLAPSEALL, OnUpdateButtonClassbCollapseall)
	ON_COMMAND(ID_BUTTON_CLASSB_COLLAPSEALL, OnButtonClassbCollapseall)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SHOW_SYSTEM, OnUpdateButtonShowSystem)
	ON_COMMAND(ID_BUTTON_SHOW_SYSTEM, OnButtonShowSystem)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FILTER, OnUpdateButtonFilter)
	ON_COMMAND(ID_BUTTON_FILTER, OnButtonFilter)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_PROPERTIES, OnUpdateButtonProperties)
	ON_COMMAND(ID_BUTTON_PROPERTIES, OnButtonProperties)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_INSTALL, OnUpdateButtonReplicInstall)
	ON_COMMAND(ID_BUTTON_REPLIC_INSTALL, OnButtonReplicInstall)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_PROPAG, OnUpdateButtonReplicPropag)
	ON_COMMAND(ID_BUTTON_REPLIC_PROPAG, OnButtonReplicPropag)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_BUILDSRV, OnUpdateButtonReplicBuildsrv)
	ON_COMMAND(ID_BUTTON_REPLIC_BUILDSRV, OnButtonReplicBuildsrv)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_RECONCIL, OnUpdateButtonReplicReconcil)
	ON_COMMAND(ID_BUTTON_REPLIC_RECONCIL, OnButtonReplicReconcil)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_DEREPLIC, OnUpdateButtonReplicDereplic)
	ON_COMMAND(ID_BUTTON_REPLIC_DEREPLIC, OnButtonReplicDereplic)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_MOBILE, OnUpdateButtonReplicMobile)
	ON_COMMAND(ID_BUTTON_REPLIC_MOBILE, OnButtonReplicMobile)
  ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_ARCCLEAN, OnUpdateButtonReplicArcclean)
	ON_COMMAND(ID_BUTTON_REPLIC_ARCCLEAN, OnButtonReplicArcclean)
  ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_REPMOD, OnUpdateButtonReplicRepmod)
	ON_COMMAND(ID_BUTTON_REPLIC_REPMOD, OnButtonReplicRepmod)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_CREATEKEYS, OnUpdateButtonReplicCreateKeys)
	ON_COMMAND(ID_BUTTON_REPLIC_CREATEKEYS, OnButtonReplicCreateKeys)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_ACTIVATE, OnUpdateButtonReplicActivate)
	ON_COMMAND(ID_BUTTON_REPLIC_ACTIVATE, OnButtonReplicActivate)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REPLIC_DEACTIVATE, OnUpdateButtonReplicDeactivate)
	ON_COMMAND(ID_BUTTON_REPLIC_DEACTIVATE, OnButtonReplicDeactivate)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_NEWWINDOW, OnUpdateButtonNewwindow)
	ON_COMMAND(ID_BUTTON_NEWWINDOW, OnButtonNewwindow)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_TEAROUT, OnUpdateButtonTearout)
	ON_COMMAND(ID_BUTTON_TEAROUT, OnButtonTearout)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_RESTARTFROMPOS, OnUpdateButtonRestartfrompos)
	ON_COMMAND(ID_BUTTON_RESTARTFROMPOS, OnButtonRestartfrompos)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_LOCATE, OnUpdateButtonLocate)
	ON_COMMAND(ID_BUTTON_LOCATE, OnButtonLocate)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESH, OnUpdateButtonRefresh)
	ON_COMMAND(ID_BUTTON_REFRESH, OnButtonRefresh)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ALTERDB, OnUpdateButtonAlterdb)
	ON_COMMAND(ID_BUTTON_ALTERDB, OnButtonAlterdb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SYSMOD, OnUpdateButtonSysmod)
	ON_COMMAND(ID_BUTTON_SYSMOD, OnButtonSysmod)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_AUDIT, OnUpdateButtonAudit)
	ON_COMMAND(ID_BUTTON_AUDIT, OnButtonAudit)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_GENSTAT, OnUpdateButtonGenstat)
	ON_COMMAND(ID_BUTTON_GENSTAT, OnButtonGenstat)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DISPSTAT, OnUpdateButtonDispstat)
	ON_COMMAND(ID_BUTTON_DISPSTAT, OnButtonDispstat)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CHECKPOINT, OnUpdateButtonCheckpoint)
	ON_COMMAND(ID_BUTTON_CHECKPOINT, OnButtonCheckpoint)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ROLLFORWARD, OnUpdateButtonRollforward)
	ON_COMMAND(ID_BUTTON_ROLLFORWARD, OnButtonRollforward)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_VERIFYDB, OnUpdateButtonVerifydb)
	ON_COMMAND(ID_BUTTON_VERIFYDB, OnButtonVerifydb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_UNLOADDB, OnUpdateButtonUnloaddb)
	ON_COMMAND(ID_BUTTON_UNLOADDB, OnButtonUnloaddb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_COPYDB, OnUpdateButtonCopydb)
	ON_COMMAND(ID_BUTTON_COPYDB, OnButtonCopydb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_POPULATE, OnUpdateButtonPopulate)
	ON_COMMAND(ID_BUTTON_POPULATE, OnButtonPopulate)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_GRANT, OnUpdateButtonGrant)
	ON_COMMAND(ID_BUTTON_GRANT, OnButtonGrant)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REVOKE, OnUpdateButtonRevoke)
	ON_COMMAND(ID_BUTTON_REVOKE, OnButtonRevoke)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_MODIFYSTRUCT, OnUpdateButtonModifystruct)
	ON_COMMAND(ID_BUTTON_MODIFYSTRUCT, OnButtonModifystruct)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_CREATEIDX, OnUpdateButtonCreateidx)
	ON_COMMAND(ID_BUTTON_CREATEIDX, OnButtonCreateidx)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_LOAD, OnUpdateButtonLoad)
	ON_COMMAND(ID_BUTTON_LOAD, OnButtonLoad)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_UNLOAD, OnUpdateButtonUnload)
	ON_COMMAND(ID_BUTTON_UNLOAD, OnButtonUnload)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DOWNLOAD, OnUpdateButtonDownload)
	ON_COMMAND(ID_BUTTON_DOWNLOAD, OnButtonDownload)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_RUNSCRIPTS, OnUpdateButtonRunscripts)
	ON_COMMAND(ID_BUTTON_RUNSCRIPTS, OnButtonRunscripts)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_UPDSTAT, OnUpdateButtonUpdstat)
	ON_COMMAND(ID_BUTTON_UPDSTAT, OnButtonUpdstat)
	ON_COMMAND(ID_BUTTON_REGISTERASLINK, OnButtonRegisteraslink)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REGISTERASLINK, OnUpdateButtonRegisteraslink)
	ON_COMMAND(ID_BUTTON_REFRESHLINK, OnButtonRefreshlink)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESHLINK, OnUpdateButtonRefreshlink)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REMOVEOBJECT, OnUpdateButtonRemoveObject)
	ON_COMMAND(ID_BUTTON_REMOVEOBJECT, OnButtonRemoveObject)
	ON_WM_DESTROY()
	ON_COMMAND(ID_BUTTON_FASTLOAD, OnButtonFastload)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_FASTLOAD, OnUpdateButtonFastload)
	ON_COMMAND(ID_BUTTON_INFODB, OnButtonInfodb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_INFODB, OnUpdateButtonInfodb)
	ON_COMMAND(ID_BUTTON_DUPLICATEDB, OnButtonDuplicateDb)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_DUPLICATEDB, OnUpdateButtonDuplicateDb)
	ON_COMMAND(ID_BUTTON_EXPIREDATE, OnButtonExpireDate)
	ON_COMMAND(ID_BUTTON_COMMENTS, OnButtonComment)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_EXPIREDATE, OnUpdateButtonExpireDate)
	ON_COMMAND(ID_BUTTON_SECURITYAUDIT, OnButtonSecurityAudit)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SECURITYAUDIT, OnUpdateButtonSecurityAudit)
	ON_WM_SIZE()
	ON_COMMAND(ID_BUTTON_JOURNALING, OnButtonJournaling)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_JOURNALING, OnUpdateButtonJournaling)
	ON_WM_CLOSE()
	ON_UPDATE_COMMAND_UI(ID_BUTTON_COMMENTS, OnUpdateButtonComment)
	ON_COMMAND(ID_BUTTON_USERMOD, OnButtonUsermod)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_USERMOD, OnUpdateButtonUsermod)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_EXPORT, OnUpdateButtonExport)
	ON_COMMAND(ID_BUTTON_EXPORT, OnButtonExport)
	//}}AFX_MSG_MAP

  // Hand-made handlers for toolbar components
  ON_BN_CLICKED   (ID_BUTTON_SYSOBJECT, OnCheckBoxSysObjClicked )
  ON_CBN_DROPDOWN (ID_BUTTON_FILTER1,   OnComboOwnerDropdown    )
  ON_CBN_SELCHANGE(ID_BUTTON_FILTER1,   OnComboOwnerSelchange   )
  ON_CBN_DROPDOWN (ID_BUTTON_FILTER2,   OnComboOtherDropdown    )
  ON_CBN_SELCHANGE(ID_BUTTON_FILTER2,   OnComboOtherSelchange   )
  ON_CBN_DROPDOWN (ID_BUTTON_FILTER3,   OnComboOtherDropdown    )
  ON_CBN_SELCHANGE(ID_BUTTON_FILTER3,   OnComboOtherSelchange   )

  // hand-made special management
  ON_WM_INITMENUPOPUP()
  ON_WM_SYSCOMMAND()

  // Toolbar management
  ON_MESSAGE(WM_USER_HASTOOLBAR,        OnHasToolbar      )
  ON_MESSAGE(WM_USER_ISTOOLBARVISIBLE,  OnIsToolbarVisible)
  ON_MESSAGE(WM_USER_SETTOOLBARSTATE,   OnSetToolbarState )
  ON_MESSAGE(WM_USER_SETTOOLBARCAPTION, OnSetToolbarCaption)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
  // TODO: add member initialization code here

  m_pDomView2         = NULL;   // view in right pane
  m_bAllViewCreated   = FALSE;
  m_bActive           = FALSE;  // doc active ?
  m_bInside           = FALSE;  // mouse cursor inside doc ?

  m_pWndSplitter      = NULL;

  m_bRegularToolbar   = TRUE;
}

CChildFrame::~CChildFrame()
{
  if (m_pWndSplitter)
    delete m_pWndSplitter;
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
  // if unicenter driven: preset maximized state, if requested
  if (IsUnicenterDriven()) {
    CuCmdLineParse* pCmdLineParse = GetAutomatedGeneralDescriptor();
    ASSERT (pCmdLineParse);
    if (pCmdLineParse->DoWeMaximizeWin()) {
      // Note: Maximize MUST be combined with VISIBLE
      cs.style |= WS_MAXIMIZE | WS_VISIBLE;
		if (!theApp.CanCloseContextDrivenFrame())
		{
			cs.style &=~WS_SYSMENU;
		}
    }
  }

  return CMDIChildWnd::PreCreateWindow(cs);
}

BOOL CChildFrame::SpecialCmdAddAlterDrop(CuDomObjDesc* pObjDesc)
{
	BOOL bContinue = FALSE;
	LPDOMDATA lpDomData = GetLPDomData(this);
	ASSERT (lpDomData);
	if (!(lpDomData || lpDomData->hwndTreeLb))
		return bContinue;

	HWND hTreeCtrl = (HWND)::SendMessage(lpDomData->hwndTreeLb, LM_GET_TREEVIEW_HWND, 0, 0);
	ASSERT (hTreeCtrl);
	if (!hTreeCtrl)
		return bContinue;
	CString strDatabases;
	CString strProfiles;
	CString strUsers;
	CString strGroups;
	CString strRoles;
	CString strLocations;
	if (!strDatabases.LoadString(IDS_TREE_DATABASE_STATIC))
		strDatabases =_T("Databases");
	if (!strProfiles.LoadString(IDS_TREE_PROFILE_STATIC))
		strProfiles =_T("Profiles");
	if (!strUsers.LoadString(IDS_TREE_USER_STATIC))
		strUsers =_T("Users");
	if (!strGroups.LoadString(IDS_TREE_GROUP_STATIC))
		strGroups =_T("Groups");
	if (!strRoles.LoadString(IDS_TREE_ROLE_STATIC))
		strRoles =_T("Roles");
	if (!strLocations.LoadString(IDS_TREE_LOCATION_STATIC))
		strLocations =_T("Locations");

	BOOL bFound = FALSE;
	HTREEITEM hItem  = TreeView_GetRoot(hTreeCtrl);
	TCHAR szBuffer[MAXOBJECTNAME*2];
	TVITEM tvitem;
	memset (&tvitem, 0, sizeof(tvitem));
	tvitem.mask  = TVIF_TEXT;
	tvitem.pszText = szBuffer;
	tvitem.cchTextMax = MAXOBJECTNAME*2;

	while (!bFound && hItem)
	{
		tvitem.hItem = hItem;
		TreeView_GetItem (hTreeCtrl, &tvitem);

		switch (pObjDesc->GetObjType())
		{
		case OT_DATABASE:
			if (lstrcmpi (tvitem.pszText, strDatabases) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		case OT_PROFILE:
			if (lstrcmpi (tvitem.pszText, strProfiles) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		case OT_USER:
			if (lstrcmpi (tvitem.pszText, strUsers) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		case OT_GROUP:
			if (lstrcmpi (tvitem.pszText, strGroups) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		case OT_ROLE:
			if (lstrcmpi (tvitem.pszText, strRoles) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		case OT_LOCATION:
			if (lstrcmpi (tvitem.pszText, strLocations) == 0)
			{
				TreeView_SelectItem(hTreeCtrl, hItem);
				bFound = TRUE;
			}
			break;
		default:
			ASSERT(FALSE);
			break;
		}
		hItem  = TreeView_GetNextSibling(hTreeCtrl, hItem);
	}
	bContinue = bFound;
	
	return bContinue;
}


/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
  CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
  CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

int CChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (!m_wndToolBar.Create(this)) {
    TRACE0("Failed to create toolbar\n");
    return -1;      // fail to create
  }

  // Pick potential unicenter-driven window descriptor
  m_bRegularToolbar = TRUE;
  CuWindowDesc* pDescriptor = NULL;
  if (IsUnicenterDriven()) {
    pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
#if 0
    if (pDescriptor->HasNoMenu() || pDescriptor->HasObjectActionsOnlyMenu())
      m_bRegularToolbar = FALSE;
#endif
  }

    if (GetOIVers() == OIVERS_NOTOI) {
      if (!m_wndToolBar.LoadToolBar(IDR_DOM_NOTOI)) {
        TRACE0("Failed to load toolbar\n");
        return -1;      // fail to create
      }

      // No comboboxes for idms/datacomm ---> goes to "System Object" checkbox code
      CRect rch1 (-80, -20, 0, 0);
      CString csSystem = _T("System Obj");
      AdjustRectForToolbar(&m_wndToolBar, csSystem, rch1);
      if (!m_CheckBoxSysObject.Create(csSystem, WS_CHILD, rch1, &m_wndToolBar,
          ID_BUTTON_SYSOBJECT)) {
        return -1;
      }
      m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_ALIGN_TOP);
      m_wndToolBar.SetButtonInfo(6, ID_BUTTON_SYSOBJECT, TBBS_SEPARATOR, -rch1.left);
      if (m_CheckBoxSysObject.m_hWnd != NULL) {
        m_wndToolBar.GetItemRect(6, rch1);
        m_CheckBoxSysObject.SetWindowPos(NULL, rch1.left, rch1.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
        m_CheckBoxSysObject.ShowWindow(SW_SHOW);
      }
    }
    else {
      if (!m_wndToolBar.LoadToolBar(IDR_DOM)) {
        TRACE0("Failed to load toolbar\n");
        return -1;      // fail to create
      } 

      // Create a ComboBox in the toolbar (database owner)
      CRect rect(-100, -80, 0, 0);
      AdjustRectForToolbar(&m_wndToolBar, VDBA_MfcResourceString(IDS_ALL_OWNERS), rect);//_T("< All Owners >")
      if (!m_ComboOwnerFilter.Create(WS_CHILD | CBS_DROPDOWNLIST |
          CBS_AUTOHSCROLL | WS_VSCROLL | CBS_HASSTRINGS, rect, &m_wndToolBar,
          ID_BUTTON_FILTER1)) {
        return -1;
      }
      m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_ALIGN_TOP);
      m_wndToolBar.SetButtonInfo(6, ID_BUTTON_FILTER1, TBBS_SEPARATOR, -rect.left);
      if (m_ComboOwnerFilter.m_hWnd != NULL) {
        m_wndToolBar.GetItemRect(6, rect);
        m_ComboOwnerFilter.SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
        m_ComboOwnerFilter.ShowWindow(SW_SHOW);
      }

      // Create a ComboBox in the toolbar (other objects owner)
      rect = CRect(-100, -80, 0, 0);
      AdjustRectForToolbar(&m_wndToolBar, VDBA_MfcResourceString(IDS_ALL_OWNERS), rect);
      if (!m_ComboOwnerFilter2.Create(WS_CHILD | CBS_DROPDOWNLIST |
          CBS_AUTOHSCROLL | WS_VSCROLL | CBS_HASSTRINGS, rect, &m_wndToolBar,
          ID_BUTTON_FILTER2)) {
        return -1;
      }
      m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_ALIGN_TOP);
      m_wndToolBar.SetButtonInfo(8, ID_BUTTON_FILTER2, TBBS_SEPARATOR, -rect.left);
      if (m_ComboOwnerFilter2.m_hWnd != NULL) {
        m_wndToolBar.GetItemRect(8, rect);
        m_ComboOwnerFilter2.SetWindowPos(NULL, rect.left, rect.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
        m_ComboOwnerFilter2.ShowWindow(SW_SHOW);
      }
      // Create a CheckBox in the toolbar (System Object)
      CRect rch1 (-80, -20, 0, 0);
      CString csSystem = _T("System Obj");
      AdjustRectForToolbar(&m_wndToolBar, csSystem, rch1);
      if (!m_CheckBoxSysObject.Create(csSystem, WS_CHILD, rch1, &m_wndToolBar,
          ID_BUTTON_SYSOBJECT)) {
        return -1;
      }
      m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_ALIGN_TOP);
      m_wndToolBar.SetButtonInfo(10, ID_BUTTON_SYSOBJECT, TBBS_SEPARATOR, -rch1.left);
      if (m_CheckBoxSysObject.m_hWnd != NULL) {
        m_wndToolBar.GetItemRect(10, rch1);
        m_CheckBoxSysObject.SetWindowPos(NULL, rch1.left, rch1.top, 0, 0, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOCOPYBITS);
        m_CheckBoxSysObject.ShowWindow(SW_SHOW);
      }
    }

  m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
  // TODO: Delete these three lines if you don't want the toolbar to
  // be dockable
  m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
  EnableDocking(CBRS_ALIGN_ANY);
  DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);

  // Set caption to toolbar
	CString str;
	str.LoadString (IDS_DOMBAR_TITLE);
	m_wndToolBar.SetWindowText (str);

  // Emb : Moved this section from beginning so that toolbar controls exist
  // when dom created, in order to have their m_hWnd stored in the dom structure
  if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
    return -1;

  return 0;
}


void CChildFrame::OnCheckBoxSysObjClicked()
{
  DomOnCommand(GetVdbaDocumentHandle(m_hWnd),
               0,  // id set to null, see ManageNotifMsg in dom.c
               m_CheckBoxSysObject.m_hWnd,
               BN_CLICKED
               );
}

void CChildFrame::OnComboOwnerDropdown()
{
  DomOnCommand(GetVdbaDocumentHandle(m_hWnd),
               0,  // id set to null, see ManageNotifMsg in dom.c
               m_ComboOwnerFilter.m_hWnd,
               CBN_DROPDOWN
               );
}

void CChildFrame::OnComboOwnerSelchange()
{
  DomOnCommand(GetVdbaDocumentHandle(m_hWnd),
               0,  // id set to null, see ManageNotifMsg in dom.c
               m_ComboOwnerFilter.m_hWnd,
               CBN_SELCHANGE
               );
}

void CChildFrame::OnComboOtherDropdown()
{
  DomOnCommand(GetVdbaDocumentHandle(m_hWnd),
               0,  // id set to null, see ManageNotifMsg in dom.c
               m_ComboOwnerFilter2.m_hWnd,
               CBN_DROPDOWN
               );
}

void CChildFrame::OnComboOtherSelchange()
{
  DomOnCommand(GetVdbaDocumentHandle(m_hWnd),
               0,  // id set to null, see ManageNotifMsg in dom.c
               m_ComboOwnerFilter2.m_hWnd,
               CBN_SELCHANGE
               );
}

void CChildFrame::OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu )
{
  DomOnInitMenuPopup(GetVdbaDocumentHandle(m_hWnd),
                     pPopupMenu->m_hMenu,
                     nIndex,
                     bSysMenu);
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd) 
{
	CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

  m_bActive = bActivate;

  DomOnMdiActivate(GetVdbaDocumentHandle(m_hWnd),
                   bActivate,
                   pActivateWnd!=0   ? GetVdbaDocumentHandle(pActivateWnd->m_hWnd)   : 0,
                   pDeactivateWnd!=0 ? GetVdbaDocumentHandle(pDeactivateWnd->m_hWnd) : 0);
}


LONG CChildFrame::OnHasToolbar(WPARAM wParam, LPARAM lParam)
{
  return (LONG)TRUE;
}

LONG CChildFrame::OnIsToolbarVisible(WPARAM wParam, LPARAM lParam)
{
  return (LONG)m_wndToolBar.IsVisible();
}

LONG CChildFrame::OnSetToolbarState(WPARAM wParam, LPARAM lParam)
{
  // lParam : TRUE means UpdateImmediate
  BOOL bDelay = (lParam ? FALSE : TRUE);
  if (wParam)
    ShowControlBar(&m_wndToolBar, TRUE, bDelay);
  else
    ShowControlBar(&m_wndToolBar, FALSE, bDelay);
  return (LONG)TRUE;
}

LONG CChildFrame::OnSetToolbarCaption(WPARAM wParam, LPARAM lParam)
{
  m_wndToolBar.SetWindowText((LPCSTR)lParam);
  return (LONG)TRUE;
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
  m_pDomView2 = RUNTIME_CLASS(CMainMfcView2);

  // Pick potential unicenter-driven window descriptor
  CuWindowDesc* pDescriptor = NULL;
  if (IsUnicenterDriven()) {
    pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
  }

  // Create splitbar of relevant type
  if (IsUnicenterDriven()) {
    if (pDescriptor->IsSplitbarNotMoveable()) {
      m_pWndSplitter = new CuQueryMainSplitterWnd;
      ((CuQueryMainSplitterWnd*)m_pWndSplitter)->SetTracking(FALSE);
    }
    else
      m_pWndSplitter = new CSplitterWnd;
  }
  else
    m_pWndSplitter = new CSplitterWnd;
  ASSERT (m_pWndSplitter);

  // Create a splitter of 1 rows and 2 columns.
    if (!m_pWndSplitter->CreateStatic (this, 1, 2)) {
    TRACE0 ("CChildFrame::OnCreateClient: Failed to create Splitter\n");
    return FALSE;
  }

  // associate the default view with pane 0
  if (!m_pWndSplitter->CreateView (0, 0, pContext->m_pNewViewClass, CSize (200, 300), pContext)) {
    TRACE0 ("CChildFrame::OnCreateClient: Failed to create first pane\n");
    return FALSE;
  }

  // associate the CIpmView2 view with pane 1
  if (!m_pWndSplitter->CreateView (0, 1, m_pDomView2, CSize (200, 300), pContext)) {
    TRACE0 ("CChildFrame::OnCreateClient: Failed to create second pane\n");
    return FALSE;
  }

  // if unicenter driven: manage width percentage if specified
  if (IsUnicenterDriven()) {
    if (pDescriptor->HasSplitbarPercentage()) {
      int cxCur, cxMin;

      // set left pane width - let right pane freewheelin'
      int pct = pDescriptor->GetSplitbarPercentage();
      int idealWidthPane0 = (long)lpcs->cx * (long)pct / 100L;
      m_pWndSplitter->GetColumnInfo(0, cxCur, cxMin);
      m_pWndSplitter->SetColumnInfo(0, idealWidthPane0, cxMin);
    }
  }

  m_pWndSplitter->RecalcLayout();
  m_bAllViewCreated = TRUE;
  //don't call CMDIChildWnd::OnCreateClient(lpcs, pContext);
  return TRUE;

}

///////////////////////////////////////////////////////////////
// Utility functions for menu state and command relay

LPDOMDATA GetLPDomData(CWnd* pWnd)
{
  HWND hwndMdi = GetVdbaDocumentHandle(pWnd->m_hWnd);
  ASSERT (hwndMdi);
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  ASSERT (lpDomData);
  return lpDomData;
}

static BOOL NEAR IsNoItem(int objType, char *objName)
{
  if (objName[0]=='\0')
    return TRUE;
  else
    return FALSE;
}

BOOL CChildFrame::GetDbItemState()
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  if (HasTrueParentDB(lpDomData, NULL))
    bEnable = TRUE;                       // Only on database direct child items
  else
    bEnable = FALSE;
  // Restricted features if gateway
  if (lpDomData->ingresVer == OIVERS_NOTOI)
    bEnable = FALSE;
  
  return bEnable;
}

BOOL CChildFrame::GetDesktopItemState(BOOL bOnlyIfDb)
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (lpDomData->ingresVer==OIVERS_NOTOI)
    bEnable = FALSE;
  else {
    switch (CurItemObjType) {
      case OT_DATABASE:
      case OT_TABLE:
      case OT_INDEX:
        if (lpRecord != 0 && !IsNoItem(CurItemObjType, (char *)lpRecord->objName)) {
          // only database for load/unload
          if (bOnlyIfDb) {
            if (CurItemObjType == OT_DATABASE)
              bEnable = TRUE;
          }
          else
            bEnable = TRUE;
        }
    }
  }

  return bEnable;
}


BOOL CChildFrame::GetReplicGenericItemState(UINT nId)
{
  BOOL bEnable = TRUE;

  BOOL bMobile = FALSE;
  BOOL bBuildServer = FALSE;
  BOOL bDereplic = FALSE;
  BOOL bNeeds20OrMore = FALSE;

  switch (nId) {
    case ID_BUTTON_REPLIC_MOBILE:
      bMobile = TRUE;
      break;
    case ID_BUTTON_REPLIC_BUILDSRV:
      bBuildServer = TRUE;
      break;
    case ID_BUTTON_REPLIC_DEREPLIC:
      bDereplic = TRUE;
      break;
    case ID_BUTTON_REPLIC_CREATEKEYS:
    case ID_BUTTON_REPLIC_ACTIVATE:
      bNeeds20OrMore = TRUE;
      break;
	  case ID_BUTTON_REPLIC_INSTALL:
    case ID_BUTTON_REPLIC_PROPAG:
    case ID_BUTTON_REPLIC_RECONCIL:
    case ID_BUTTON_REPLIC_ARCCLEAN:
    case ID_BUTTON_REPLIC_REPMOD:
      break;
    default:
      ASSERT (FALSE);
      return FALSE;
  }

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  bEnable = FALSE;
  if (CurItemObjType == OT_STATIC_REPLICATOR) {
    if (lpDomData->iReplicVersion!=REPLIC_NOINSTALL) {
      int ret = ReplicatorLocalDbInstalled(lpDomData->psDomNode->nodeHandle,
                                           lpRecord->extra,lpDomData->iReplicVersion);
      switch (ret) {
        case RES_SUCCESS:
          bEnable = TRUE; // as a default
          if (bMobile) {
            if (lpDomData->iReplicVersion != REPLIC_V105)
              bEnable = FALSE;
          }
          if (bBuildServer) {
            if (lpDomData->ingresVer >= OIVERS_25)
              bEnable = FALSE;
          }
          if (bNeeds20OrMore) {
            if (lpDomData->ingresVer < OIVERS_20)
              bEnable = FALSE;
          }
          break;

        case RES_ERR:
          if (bDereplic)
            bEnable = TRUE;
          else
            bEnable = FALSE;
          break;

        case RES_NOGRANT:
        case RES_SQLNOEXEC:   // database does not exist anymore!
          bEnable = FALSE;
          break;

        default:
          ASSERT (FALSE);     // Unexepected RES_BLAHBLAH
          bEnable = FALSE;
          break;
      }
    }
  }
  
  return bEnable;
}

void CChildFrame::RelayCommand(UINT itemId)
{
    CMainFrame*  pMain = (CMainFrame*)AfxGetMainWnd();
    WPARAM wParam = MAKEWPARAM((UINT)(itemId), 0);
    LPARAM lParam = (LPARAM)0;
    pMain->RelayCommand (wParam, lParam);
}


///////////////////////////////////////////////////////////////
// Handlers for menu state and command relay

void CChildFrame::OnUpdateButtonSpacecalc(CCmdUI* pCmdUI)
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);

  if (lpDomData->ingresVer==OIVERS_NOTOI)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnUpdateButtonAddobject(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // Add object : special management
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int potAddType;

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
    bEnable = FALSE;
  else {
    if (bCurSelStatic) {
      DWORD dwFirstChild = GetFirstImmediateChild(lpDomData, dwCurSel, 0);
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
      bEnable = TRUE;
    else
      bEnable = FALSE;
  }

  // Emb 15/11/95 - post-manage gray for non-static at root level
  if (::SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwCurSel)==0
      && !bCurSelStatic)
    bEnable = FALSE;

  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI) {
    if (CurItemObjType == OT_STATIC_DATABASE || CurItemObjType == OT_DATABASE)
      bEnable = FALSE;
  }

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
    if (CurItemObjType == OT_PROCEDURE || CurItemObjType == OT_STATIC_PROCEDURE)
      bEnable = FALSE;
    if (CurItemObjType == OT_INDEX || CurItemObjType == OT_STATIC_INDEX)
      bEnable = FALSE;
  }

  // Post correction for STAR CDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_COORDINATOR) {
    // No Add/Alter/Drop/Paste for tables, views and procedures
    if (CurItemObjType == OT_TABLE || CurItemObjType == OT_STATIC_TABLE || 
        CurItemObjType == OT_VIEW  || CurItemObjType == OT_STATIC_VIEW  ||
        CurItemObjType == OT_PROCEDURE  || CurItemObjType == OT_STATIC_PROCEDURE) {
      bEnable = FALSE;
    }
  }

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonAddobject() 
{
  RelayCommand(ID_BUTTON_ADDOBJECT);
  TRACE0 ("CChildFrame::OnButtonAddobject() ...\n");	
}

void CChildFrame::OnUpdateButtonAlterobject(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (bCurSelStatic)
    bEnable = FALSE;
  else {
    if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
      bEnable = FALSE;
    else {
      // special management due to load feature, added 21/11/96
      int oldOIVers = GetOIVers();
      SetOIVers(lpDomData->ingresVer);
      if (CanObjectBeAltered(CurItemObjType, lpRecord->objName,
                                            lpRecord->ownerName) 
	&& (!IsVW() 
	|| (IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0))
	)
        bEnable = TRUE;
      else
        bEnable = FALSE;
      SetOIVers(oldOIVers);
    }
  }
  // Emb 15/11/95 - post-manage gray for non-static at root level
  if (::SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwCurSel)==0
      && !bCurSelStatic)
    bEnable = FALSE;

  // Post correction for STAR DDB : No alter whatever the item type is
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;

  // Post correction for STAR CDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_COORDINATOR) {
    // No Add/Alter/Drop/Paste for tables, views and procedures
    if (CurItemObjType == OT_TABLE || CurItemObjType == OT_STATIC_TABLE || 
        CurItemObjType == OT_VIEW  || CurItemObjType == OT_STATIC_VIEW  ||
        CurItemObjType == OT_PROCEDURE  || CurItemObjType == OT_STATIC_PROCEDURE) {
      bEnable = FALSE;
    }
  }
  
  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonAlterobject() 
{
  RelayCommand(ID_BUTTON_ALTEROBJECT);
  TRACE0 ("CChildFrame::OnButtonAlterobject() ...\n");	
}

void CChildFrame::OnUpdateButtonDropobject(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (bCurSelStatic)
    bEnable = FALSE;
  else {
    if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
      bEnable = FALSE;
    else
      if (CanObjectBeDeleted(CurItemObjType, lpRecord->objName,
                                            lpRecord->ownerName))
        bEnable = TRUE;
      else
        bEnable = FALSE;
  }

  // Emb 15/11/95 - post-manage gray for non-static at root level
  if (::SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL, 0, dwCurSel)==0
      && !bCurSelStatic)
    bEnable = FALSE;

  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI) {
    if (CurItemObjType == OT_STATIC_DATABASE || CurItemObjType == OT_DATABASE)
      bEnable = FALSE;
  }

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
    switch (CurItemObjType) {
      case OT_INDEX:
        bEnable = FALSE;
        break;
      case OT_TABLE:
        if (getint(lpRecord->szComplim + STEPSMALLOBJ) != OBJTYPE_STARNATIVE)
          bEnable = FALSE;
        break;
      case OT_VIEW:
        if (getint(lpRecord->szComplim + 4 + STEPSMALLOBJ) != OBJTYPE_STARNATIVE)
          bEnable = FALSE;
        break;
      case OT_PROCEDURE:
          bEnable = FALSE;    // Always registered as link
        break;
    }
  }

  // Post correction for STAR CDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_COORDINATOR) {
    // No Add/Alter/Drop/Paste for tables, views and procedures
    if (CurItemObjType == OT_TABLE || CurItemObjType == OT_STATIC_TABLE || 
        CurItemObjType == OT_VIEW  || CurItemObjType == OT_STATIC_VIEW  ||
        CurItemObjType == OT_PROCEDURE  || CurItemObjType == OT_STATIC_PROCEDURE) {
      bEnable = FALSE;
    }
  }
  
  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonDropobject() 
{
  RelayCommand(ID_BUTTON_DROPOBJECT);
  TRACE0 ("CChildFrame::OnButtonDropobject() ...\n");	
}


void CChildFrame::OnUpdateButtonRemoveObject(CCmdUI* pCmdUI) 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  BOOL bEnable = FALSE;   // DEFAULTS TO FALSE, UNLIKE OTHER OnUpdate() Methods
  if (!bCurSelStatic) {
    if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED) {
      switch (CurItemObjType) {
        case OT_TABLE:
          if (getint(lpRecord->szComplim + STEPSMALLOBJ) == OBJTYPE_STARLINK)
            bEnable = TRUE;
          break;
        case OT_VIEW:
          if (getint(lpRecord->szComplim + 4 + STEPSMALLOBJ) == OBJTYPE_STARLINK)
            bEnable = TRUE;
          break;
        case OT_PROCEDURE:
            bEnable = TRUE;    // Always registered as link
          break;
      }
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRemoveObject() 
{
  RelayCommand(ID_BUTTON_REMOVEOBJECT);
  TRACE0 ("CChildFrame::OnButtonRemoveObject() ...\n");	
}



void CChildFrame::OnUpdateButtonPrint(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonPrint() 
{
  RelayCommand(ID_BUTTON_PRINT);
}

void CChildFrame::OnUpdateButtonFind(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;
  if (hDlgSearch)       // 'search' dialog in progress' - hDlgSearch defined in main.c
    bEnable = FALSE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonFind() 
{
  RelayCommand(ID_BUTTON_FIND);
}

void CChildFrame::OnUpdateButtonClassbExpandone(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbExpandone() 
{
  RelayCommand(ID_BUTTON_CLASSB_EXPANDONE);
}

void CChildFrame::OnUpdateButtonClassbExpandbranch(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbExpandbranch() 
{
  RelayCommand(ID_BUTTON_CLASSB_EXPANDBRANCH);
}

void CChildFrame::OnUpdateButtonClassbExpandall(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbExpandall() 
{
  RelayCommand(ID_BUTTON_CLASSB_EXPANDALL);
}

void CChildFrame::OnUpdateButtonClassbCollapseone(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbCollapseone() 
{
  RelayCommand(ID_BUTTON_CLASSB_COLLAPSEONE);
}

void CChildFrame::OnUpdateButtonClassbCollapsebranch(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbCollapsebranch() 
{
  RelayCommand(ID_BUTTON_CLASSB_COLLAPSEBRANCH);
}

void CChildFrame::OnUpdateButtonClassbCollapseall(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonClassbCollapseall() 
{
  RelayCommand(ID_BUTTON_CLASSB_COLLAPSEALL);
}

void CChildFrame::OnUpdateButtonShowSystem(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;
  pCmdUI->Enable(bEnable);

  int checkState; // 0 unchecked, 1 checked
  // No "::GetParent(m_hWnd)" anymore since moved from View to Frame
  checkState = DomIsSystemChecked(m_hWnd) ? 1 : 0;
  pCmdUI->SetCheck(checkState);
}

void CChildFrame::OnButtonShowSystem() 
{
  RelayCommand(ID_BUTTON_SHOW_SYSTEM);
}

void CChildFrame::OnUpdateButtonFilter(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonFilter() 
{
  RelayCommand(ID_BUTTON_FILTER);
}

void CChildFrame::OnUpdateButtonProperties(CCmdUI* pCmdUI) 
{
  ASSERT (FALSE);   // Obsolete since in right pane of dom
  BOOL bEnable = TRUE;
  pCmdUI->Enable(bEnable);
  //  pCmdUI->Enable(GetMenuItemState(ID_BUTTON_PROPERTIES));
}

void CChildFrame::OnButtonProperties() 
{
  ASSERT (FALSE);   // Obsolete since in right pane of dom
  // RelayCommand(ID_BUTTON_PROPERTIES);
}

void CChildFrame::OnUpdateButtonReplicInstall(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  bEnable = FALSE;
  if (CurItemObjType == OT_STATIC_REPLICATOR) {
    if (lpDomData->iReplicVersion==REPLIC_NOINSTALL)
      bEnable = TRUE;
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicInstall() 
{
  RelayCommand(ID_BUTTON_REPLIC_INSTALL);
}

void CChildFrame::OnUpdateButtonReplicPropag(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_PROPAG);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicPropag() 
{
  RelayCommand(ID_BUTTON_REPLIC_PROPAG);
}

void CChildFrame::OnUpdateButtonReplicBuildsrv(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_BUILDSRV);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicBuildsrv() 
{
  RelayCommand(ID_BUTTON_REPLIC_BUILDSRV);
}

void CChildFrame::OnUpdateButtonReplicReconcil(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_RECONCIL);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicReconcil() 
{
  RelayCommand(ID_BUTTON_REPLIC_RECONCIL);
}

void CChildFrame::OnUpdateButtonReplicDereplic(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_DEREPLIC);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicDereplic() 
{
  RelayCommand(ID_BUTTON_REPLIC_DEREPLIC);
}

void CChildFrame::OnUpdateButtonReplicMobile(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_MOBILE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicMobile() 
{
  RelayCommand(ID_BUTTON_REPLIC_MOBILE);
}

void CChildFrame::OnUpdateButtonReplicArcclean(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_ARCCLEAN);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicArcclean() 
{
  RelayCommand(ID_BUTTON_REPLIC_ARCCLEAN);
}

void CChildFrame::OnUpdateButtonReplicRepmod(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetReplicGenericItemState(ID_BUTTON_REPLIC_REPMOD);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicRepmod()
{
  RelayCommand(ID_BUTTON_REPLIC_REPMOD);
}

void CChildFrame::OnUpdateButtonReplicCreateKeys(CCmdUI* pCmdUI)
{
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  if (dwCurSel) {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
    if (lpRecord && lpDomData->ingresVer >= OIVERS_25) {   // Replicator on Ingres 2.5 specific
      if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName)) {
        switch (lpRecord->recType) {
          case OTR_REPLIC_CDDS_TABLE:
          case OT_REPLIC_REGTABLE:
            bEnable = TRUE;
            break;
        }
      }
    }
  }
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicCreateKeys()
{
  RelayCommand(ID_BUTTON_REPLIC_CREATEKEYS);
}

void CChildFrame::OnUpdateButtonReplicActivate(CCmdUI* pCmdUI)
{
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  if (dwCurSel) {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
    if (lpRecord && lpDomData->ingresVer >= OIVERS_25) {   // Replicator on Ingres 2.5 specific
      if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName)) {
        switch (lpRecord->recType) {
          case OT_REPLIC_CDDS:
          case OTR_REPLIC_TABLE_CDDS:
          case OTR_REPLIC_CDDS_TABLE:
          case OT_REPLIC_REGTABLE:
            bEnable = TRUE;
            break;
        }
      }
    }
  }
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicActivate() 
{
  RelayCommand(ID_BUTTON_REPLIC_ACTIVATE);
}

void CChildFrame::OnUpdateButtonReplicDeactivate(CCmdUI* pCmdUI)
{
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  if (dwCurSel) {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
    if (lpRecord && lpDomData->ingresVer >= OIVERS_25) {   // Replicator on Ingres 2.5 specific
      if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName)) {
        switch (lpRecord->recType) {
          case OT_REPLIC_CDDS:
          case OTR_REPLIC_TABLE_CDDS:
          case OTR_REPLIC_CDDS_TABLE:
          case OT_REPLIC_REGTABLE:
            bEnable = TRUE;
            break;
        }
      }
    }
  }
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonReplicDeactivate() 
{
  RelayCommand(ID_BUTTON_REPLIC_DEACTIVATE);
}

void CChildFrame::OnUpdateButtonNewwindow(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonNewwindow() 
{
  RelayCommand(ID_BUTTON_NEWWINDOW);
}

void CChildFrame::OnUpdateButtonTearout(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonTearout() 
{
  RelayCommand(ID_BUTTON_TEAROUT);
}

void CChildFrame::OnUpdateButtonRestartfrompos(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRestartfrompos() 
{
  RelayCommand(ID_BUTTON_RESTARTFROMPOS);
}

void CChildFrame::OnUpdateButtonLocate(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonLocate() 
{
  RelayCommand(ID_BUTTON_LOCATE);
}

void CChildFrame::OnUpdateButtonRefresh(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  // if context-driven with restricted menu ---> gray buttons
  if (!m_bRegularToolbar)
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRefresh() 
{
  RelayCommand(ID_BUTTON_REFRESH);
}

void CChildFrame::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
  BOOL bEnable = FALSE;   // Never cut
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnEditCut() 
{
  RelayCommand(ID_EDIT_CUT);
}

void CChildFrame::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (bCurSelStatic)
    bEnable = FALSE;
  else {
    if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
      bEnable = FALSE;
    else
      if (CanObjectBeCopied(CurItemObjType, lpRecord->objName, lpRecord->ownerName))
        bEnable = TRUE;
      else
        bEnable = FALSE;
  }

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;
  
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnEditCopy() 
{
  RelayCommand(ID_EDIT_COPY);
}

void CChildFrame::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (DomClipboardObjType) {
    int potAddType;
    if (bCurSelStatic) {
      DWORD dwFirstChild = GetFirstImmediateChild(lpDomData, dwCurSel, 0);
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
      bEnable = FALSE;
    else
      bEnable = TRUE;
  }
  else
    bEnable = FALSE;

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;

  // Post correction for STAR CDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_COORDINATOR) {
    // No Add/Alter/Drop/Paste for tables, views and procedures
    if (CurItemObjType == OT_TABLE || CurItemObjType == OT_STATIC_TABLE || 
        CurItemObjType == OT_VIEW  || CurItemObjType == OT_STATIC_VIEW  ||
        CurItemObjType == OT_PROCEDURE  || CurItemObjType == OT_STATIC_PROCEDURE) {
      bEnable = FALSE;
    }
  }
  
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnEditPaste() 
{
  RelayCommand(ID_EDIT_PASTE);
}

void CChildFrame::OnUpdateButtonAlterdb(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  if (!CanObjectExistInVectorWise(CurItemObjType))
	  bEnable=FALSE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonAlterdb() 
{
  RelayCommand(ID_BUTTON_ALTERDB);
}

void CChildFrame::OnUpdateButtonSysmod(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  //BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  if (!CanObjectExistInVectorWise(CurItemObjType))
	  bEnable=FALSE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonSysmod() 
{
  RelayCommand(ID_BUTTON_SYSMOD);
}

void CChildFrame::OnUpdateButtonAudit(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  if (!CanObjectExistInVectorWise(CurItemObjType))
	  bEnable=FALSE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonAudit() 
{
  RelayCommand(ID_BUTTON_AUDIT);
}

void CChildFrame::OnUpdateButtonGenstat(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonGenstat() 
{
  RelayCommand(ID_BUTTON_GENSTAT);
}

void CChildFrame::OnUpdateButtonDispstat(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonDispstat() 
{
  RelayCommand(ID_BUTTON_DISPSTAT);
}

void CChildFrame::OnUpdateButtonCheckpoint(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType;
  if ((IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) > 0)
	  || CurItemObjType == OT_DATABASE || CurItemObjType == OT_STATIC_TABLE)
	  bEnable=FALSE;
    pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonCheckpoint() 
{
  RelayCommand(ID_BUTTON_CHECKPOINT);
}

void CChildFrame::OnUpdateButtonRollforward(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType;
  if ((IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) > 0)
	  || CurItemObjType == OT_DATABASE || CurItemObjType == OT_STATIC_TABLE)
	  bEnable = FALSE;
    pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRollforward() 
{
  RelayCommand(ID_BUTTON_ROLLFORWARD);
  RelayCommand(ID_BUTTON_REFRESH);
}

void CChildFrame::OnUpdateButtonVerifydb(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic (lpDomData);
  if (!CanObjectExistInVectorWise(CurItemObjType))
	  bEnable=FALSE;
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonVerifydb() 
{
  RelayCommand(ID_BUTTON_VERIFYDB);
}

void CChildFrame::OnUpdateButtonUnloaddb(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonUnloaddb() 
{
  RelayCommand(ID_BUTTON_UNLOADDB);
}

void CChildFrame::OnUpdateButtonCopydb(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDbItemState();
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonCopydb()
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  if (lpDomData->ingresVer>=OIVERS_26)
  {
    DWORD dwCurSel = GetCurSel(lpDomData);
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
    int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
    CxDlgCopyDb dlg;

    if (CurItemObjType == OT_TABLE)
    {
      dlg.m_csDBName = lpRecord->extra;
      dlg.m_csTableName  = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
      dlg.m_csTableOwner = lpRecord->ownerName;
    }
    else if (CurItemObjType == OT_DATABASE)
    {
      dlg.m_csDBName = lpRecord->objName;
      dlg.m_csTableName  = _T("");
      dlg.m_csTableOwner = _T("");
    }
    else
    {
      dlg.m_csDBName = lpRecord->extra;
      dlg.m_csTableName  = _T("");
      dlg.m_csTableOwner = _T("");
    }
    dlg.m_iObjectType = CurItemObjType;
    dlg.m_csNodeName      = (LPCTSTR)(LPUCHAR)GetVirtNodeName (lpDomData->psDomNode->nodeHandle);

    dlg.DoModal();
  }
  else
    RelayCommand(ID_BUTTON_COPYDB);
}

void CChildFrame::OnUpdateButtonPopulate(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Populate : only if item is a database or a table
  switch (CurItemObjType) {
    case OT_DATABASE:
    case OT_TABLE:
      if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName)) 
        bEnable = FALSE;
      else 
        bEnable = TRUE;
      break;
    default:
      bEnable = FALSE;
      break;
  }
  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI) {
    if (CurItemObjType == OT_STATIC_DATABASE || CurItemObjType == OT_DATABASE)
      bEnable = FALSE;
  }

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;
  
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonPopulate() 
{
  RelayCommand(ID_BUTTON_POPULATE);
}

void CChildFrame::OnUpdateButtonGrant(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  switch (CurItemObjType) {
    case OT_DATABASE:
    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:
    case OT_DBEVENT:
    case OT_SEQUENCE:
    case OT_ROLE:
      if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
        bEnable = FALSE;
      else
        bEnable = TRUE;
      break;

    default:
      bEnable = FALSE;
      break;
  }

  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI)
    bEnable = FALSE;

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;
  
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonGrant() 
{
  RelayCommand(ID_BUTTON_GRANT);
}

void CChildFrame::OnUpdateButtonRevoke(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  switch (CurItemObjType) {
    case OT_DATABASE:
    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
    case OT_SCHEMAUSER_TABLE:
    case OT_SCHEMAUSER_VIEW:
    case OT_SCHEMAUSER_PROCEDURE:
    case OT_DBEVENT:
    case OT_SEQUENCE:
    case OT_ROLE:
      if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
        bEnable = FALSE;
      else
        bEnable = TRUE;
      break;

    default:
      bEnable = FALSE;
      break;
  }

  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI)
    bEnable = FALSE;

  // Post correction for STAR DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;
  
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRevoke() 
{
  RelayCommand(ID_BUTTON_REVOKE);
}

void CChildFrame::OnUpdateButtonModifystruct(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName))
    bEnable = FALSE;
  else
    if (CanObjectStructureBeModified(CurItemObjType))
      bEnable = TRUE;
    else
      bEnable = FALSE;

  // Restricted features if gateway
  if (lpDomData->ingresVer==OIVERS_NOTOI)
    bEnable = FALSE;

  // Restricted feature if VectorWise
  if (lpDomData->bIsVectorWise && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) > 0)
	  bEnable = FALSE;

  // Restricted features if DDB
  if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
    bEnable = FALSE;

  // Not acceptable for system object
  if (lpRecord && IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName))
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonModifystruct() 
{
  RelayCommand(ID_BUTTON_MODIFYSTRUCT);
}

void CChildFrame::OnUpdateButtonCreateidx(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (lpDomData->bIsVectorWise && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == IDX_VW)
	  bEnable = TRUE;
  else
      bEnable = FALSE;

  // Not acceptable for system object
  if (lpRecord && IsSystemObject(CurItemObjType, lpRecord->objName, lpRecord->ownerName))
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}
void CChildFrame::OnButtonCreateidx()
{
	RelayCommand(ID_BUTTON_CREATEIDX);
}

void CChildFrame::OnUpdateButtonLoad(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDesktopItemState(TRUE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonLoad() 
{
  RelayCommand(ID_BUTTON_LOAD);
}

void CChildFrame::OnUpdateButtonUnload(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDesktopItemState(TRUE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonUnload() 
{
  RelayCommand(ID_BUTTON_UNLOAD);
}

void CChildFrame::OnUpdateButtonDownload(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDesktopItemState(TRUE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonDownload() 
{
  RelayCommand(ID_BUTTON_DOWNLOAD);
}

void CChildFrame::OnUpdateButtonRunscripts(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDesktopItemState(TRUE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRunscripts() 
{
  RelayCommand(ID_BUTTON_RUNSCRIPTS);
}

void CChildFrame::OnUpdateButtonUpdstat(CCmdUI* pCmdUI) 
{
  BOOL bEnable = GetDesktopItemState(FALSE);
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonUpdstat() 
{
  RelayCommand(ID_BUTTON_UPDSTAT);
}

void CChildFrame::OnButtonRegisteraslink() 
{
  RelayCommand(ID_BUTTON_REGISTERASLINK);
  TRACE0 ("CChildFrame::OnButtonRegisterAsLink() ...\n");	
}

void CChildFrame::OnUpdateButtonRegisteraslink(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (CanRegisterAsLink(lpDomData->psDomNode->nodeHandle, lpRecord))
    bEnable = TRUE;
  else
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonRefreshlink() 
{
  RelayCommand(ID_BUTTON_REFRESHLINK);
  TRACE0 ("CChildFrame::OnButtonRefreshLink() ...\n");	
}

void CChildFrame::OnUpdateButtonRefreshlink(CCmdUI* pCmdUI) 
{
  BOOL bEnable = TRUE;

  LPDOMDATA lpDomData = GetLPDomData(this);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  if (CanRefreshLink(lpDomData->psDomNode->nodeHandle, lpRecord))
    bEnable = TRUE;
  else
    bEnable = FALSE;

  pCmdUI->Enable(bEnable);
}


void CChildFrame::OnDestroy() 
{
  theApp.SetLastDocMaximizedState(IsZoomed());

	CMDIChildWnd::OnDestroy();
}

void CChildFrame::OnIdleGlobStatusUpdate() 
{
  // if active mdi document:
  // Check whether mouse is inside the doc.
  // If it was inside and it is not anymore,
  // only then, reset the global status
  if (m_bActive) {
    BOOL bInside = FALSE;
    POINT ptCursor;
    GetCursorPos(&ptCursor);
    CRect windowRect;
    GetWindowRect(&windowRect);
    if (windowRect.PtInRect(ptCursor))
      bInside = TRUE;
    else
      bInside = FALSE;
    if (bInside == FALSE && m_bInside == TRUE) {
      // Just go for the status refresh!
      CMainFrame* pMainFrame = (CMainFrame *)AfxGetMainWnd();
      CStatusBar *pStatusBar = pMainFrame->GetMainFrmStatusBar();
      ASSERT (pStatusBar);
      if (pStatusBar) {
        CString csText = "Ready";
        BOOL bSuccess = csText.LoadString(AFX_IDS_IDLEMESSAGE);
        ASSERT (bSuccess);
        pStatusBar->SetPaneText(0, csText);
      }
    }
    m_bInside = bInside;
  }
}

DWORD CChildFrame::GetHelpID()
{
    UINT idDomHelp    = HELPID_DOM; 
    CMainMfcView2* pView2 = (CMainMfcView2*)GetRightPane();
    ASSERT (pView2);
    if (pView2)
    {
        UINT id = pView2->GetHelpID();
        if (id > 0) idDomHelp = id;
    }
    TRACE1 ("Help ID = %u\n", idDomHelp);
    return (DWORD)idDomHelp;
}

//
// Return the image of the current selected tree's branch
HICON CChildFrame::DOMTreeGetCurrentIcon(HWND hwndDOMTree)
{
    HICON hIcon = NULL;
    HWND hwndTreeView = (HWND)::SendMessage (hwndDOMTree, LM_GET_TREEVIEW_HWND, 0, 0);
    if (!hwndTreeView)
        return NULL;
    HTREEITEM hSelectedItem = TreeView_GetSelection(hwndTreeView);
    if (hSelectedItem)
    {
        // First, get image info about selected item
        int nImage = -1, nSelectedImage = -1;
        TV_ITEM tvitem;
        memset (&tvitem, 0, sizeof(tvitem));
        tvitem.mask = TVIF_IMAGE|TVIF_SELECTEDIMAGE;
        tvitem.hItem= hSelectedItem;
        if (TreeView_GetItem (hwndTreeView, &tvitem))
        {
            nImage = tvitem.iImage;
            nSelectedImage = tvitem.iSelectedImage;
        }

        // Second, AFTER TreeView_GetItem, pick image list data
        int nImageCount = 0;
        CImageList imageList;
        HIMAGELIST hImageList = TreeView_GetImageList (hwndTreeView, TVSIL_NORMAL);
        if (!hImageList)
            return NULL;
        if (!imageList.Attach(hImageList))
            return NULL;
        nImageCount = imageList.GetImageCount();

        // Extract icon if possible
        if (nImage != -1 && nImage < nImageCount)
            hIcon = imageList.ExtractIcon(nImage);
        imageList.Detach();
    }
    return hIcon;
}

///////////////////////////////////////////////////////////////////////////////////////////
// For Fast search through double click


DWORD CChildFrame::GetTreeCurSel()
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  return dwCurSel;
}

DWORD CChildFrame::FindSearchedItem(CuDomTreeFastItem* pFastItem, DWORD dwZeSel)
{
  // pîck Fast Item relevant data
  int     reqType    = pFastItem->GetType();
  CString csReqName  = pFastItem->GetName();
  CString csReqOwner = pFastItem->GetOwner();

  // 1) expand except if dwZeSel is null
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (dwZeSel)
    ::SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0, (LPARAM)dwZeSel);

  // 2) scan children or root items until found
  DWORD dwChild;
  if (dwZeSel)
    dwChild = ::SendMessage(lpDomData->hwndTreeLb, LM_GETFIRSTCHILD, 0, (LPARAM)dwZeSel);
  else
    dwChild = ::SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0);
  ASSERT (dwChild);
  while (dwChild) {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                                         LM_GETITEMDATA, 0, (LPARAM)dwChild);
    ASSERT (lpRecord);
    if (lpRecord->recType == reqType) {
      // Pre test: if names not relevant, item is fine!
      if (csReqName.IsEmpty() && csReqOwner.IsEmpty()) {
        ASSERT (IsItemObjStatic(lpDomData, dwChild));   // expected to be of static type
        return dwChild;
      }
      ASSERT (!csReqName.IsEmpty());
      if (csReqName == (LPCTSTR)lpRecord->objName) {
        // check owner except if empty
        if (csReqOwner.IsEmpty() || (csReqOwner == (LPCTSTR)lpRecord->ownerName) ) {
          // Assertion must be masqued because of event-driven management
          // ASSERT (!IsItemObjStatic(lpDomData, dwChild) );  // expected NOT to be of static type
          return dwChild;
        }
      }
    }
    dwChild = ::SendMessage(lpDomData->hwndTreeLb, LM_GETNEXTSIBLING, 0, (LPARAM)dwChild);
  }
  ASSERT (FALSE);
  return 0L;
}

void CChildFrame::SelectItem(DWORD dwZeSel)
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  // Note: post or send seems to work quite the same
  ::SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)dwZeSel);
}

void CChildFrame::OnButtonFastload() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()

  ASSERT(CurItemObjType == OT_TABLE);

  CxDlgFastLoad dlg;

  dlg.m_csDatabaseName  = (LPCTSTR)lpRecord->extra;
  dlg.m_csTableName     = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
  dlg.m_csOwnerTbl      = (LPCTSTR)lpRecord->ownerName;
  dlg.m_csNodeName      = (LPCTSTR)(LPUCHAR)GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
  dlg.DoModal();
}

void CChildFrame::OnUpdateButtonFastload(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database on pure ingres node
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer>OIVERS_20) {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel) {
      LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                    LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->recType == OT_TABLE)
        if (lpRecord->parentDbType == DBTYPE_REGULAR)
          if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName))
            if (!IsSystemObject(lpRecord->recType, lpRecord->objName, lpRecord->ownerName) && 
				(!IsVW() || (IsVW && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0)))
	              bEnable = TRUE;
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonInfodb() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_DATABASE);

  // Pick impersonated user
  UCHAR userName[MAXOBJECTNAME];
  LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
  DBAGetUserName(vnodeName, userName);

  CxDlgInfoDB dlg;
  dlg.m_csDBname = lpRecord->objName;
  dlg.m_csOwnerName = userName;
  dlg.m_csVnodeName = vnodeName;
  dlg.DoModal();

}

void CChildFrame::OnUpdateButtonInfodb(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database ,whatever system or not.

  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer>OIVERS_20) {
      DWORD dwCurSel = GetCurSel(lpDomData);
      if (dwCurSel) {
        LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                        LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
        int curItemObjType = lpRecord->recType; // Same as GetItemObjType()
        if (curItemObjType == OT_DATABASE)
          if (lpRecord->parentDbType == DBTYPE_REGULAR)
          bEnable = TRUE;
      }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::RelayerOnButtonInfodb() 
{
  OnButtonInfodb();
}

void CChildFrame::RelayerOnButtonFastload() 
{
  OnButtonFastload();
}



void CChildFrame::RelayerOnButtonDuplicateDb() 
{
  OnButtonDuplicateDb();
}

void CChildFrame::OnButtonDuplicateDb() 
{
  int iret;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_DATABASE);

  CxDlgDuplicateDb dlg;
  dlg.m_DbType = lpRecord->parentDbType;
  dlg.m_nNodeHandle = lpDomData->psDomNode->nodeHandle;
  dlg.m_csCurrentDatabaseName  = (LPCTSTR)lpRecord->objName;
  iret = dlg.DoModal();
  if ( iret == IDCANCEL)
    return;
  // refresh
  int nodeHandle =lpDomData->psDomNode->nodeHandle;
  DOMDisableDisplay(nodeHandle, NULL);
  UpdateDOMDataAllUsers(
        nodeHandle,         // hnodestruct
        OT_DATABASE,        // iobjecttype
        0,                  // level
        NULL,               // parentstrings
        FALSE,              // keep system tbls state
        FALSE);             // bWithChildren
  DOMUpdateDisplayDataAllUsers (
        nodeHandle,         // hnodestruct
        OT_DATABASE,        // iobjecttype
        0,                  // level
        NULL,               // parentstrings
        FALSE,              // bWithChildren
        ACT_ADD_OBJECT,     // object added
        0L,                 // no item id
        0);                 // all mdi windows concerned
  DOMEnableDisplay(nodeHandle, NULL);

   // complimentary management of right pane: refresh it immediately
  HWND hwndMdi = GetVdbaDocumentHandle(m_hWnd);
  UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);

   // Fix Nov 4, 97: must Update number of objects after an add
   // (necessary since we have optimized the function that counts the tree items
   // for performance purposes)
   GetNumberOfObjects(lpDomData, TRUE);
   // display will be updated at next call to the same function

   // warn if created object not visible because of filters 
   // The source user is the same on the target database. not necessary.
#if 0
   if (NotVisibleDueToFilters(lpDomData, objType))
     MessageBox (GetFocus(),
                 _T("Object not displayed due to current filters state"),
                 _T("Add Object"),
                 MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
#endif
}

void CChildFrame::OnUpdateButtonDuplicateDb(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer>OIVERS_20) {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel) {
      LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                    LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->recType == OT_DATABASE)
        if (lpRecord->parentDbType == DBTYPE_REGULAR && !IsVW())
          bEnable = TRUE;
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::RelayerOnButtonSecurityAudit() 
{
  OnButtonSecurityAudit();
}

void CChildFrame::OnButtonSecurityAudit() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_STATIC_INSTALL_SECURITY);

  BOOL bSuccess = VDBA_EnableDisableSecurityLevel();

	//
	// Refresh the DOM Right Pane Property:
	if (bSuccess && lpDomData)
	{
		HWND hwndMdi = GetVdbaDocumentHandle(m_hWnd);
		if (hwndMdi)
			UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);
	}
}

void CChildFrame::OnUpdateButtonSecurityAudit(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  if (dwCurSel) {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
    int curItemObjType = lpRecord->recType; // Same as GetItemObjType()
    if (curItemObjType == OT_STATIC_INSTALL_SECURITY)
      bEnable = TRUE;
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::RelayerOnButtonExpireDate() 
{
  OnButtonExpireDate();
}
void CChildFrame::RelayerOnButtonComment() 
{
  OnButtonComment();
}

void CChildFrame::RelayerOnButtonUsermod()
{
  OnButtonUsermod();
}

void CChildFrame::RelayerOnButtonCopyDB()
{
  OnButtonCopydb();
}

void CChildFrame::RelayerOnButtonJournaling() 
{
  OnButtonJournaling();
}


static CString ReformatDate(LPCTSTR lpszExpireDate)
{
  CString csTemp = lpszExpireDate;
  CString csExpire = _T("");

// 14-Jan-2000 (noifr01) (bug #100015) changed the function, according to the new
// input format which is now a 4 years digit, and the time at the right of the string
// (separated by "   ")
  int index = csTemp.Find(_T("   ")); // look for the time part
  if (index > 10) {
		CString csTime= csTemp.Mid(index+3); //additional check for valid input format
		if (csTime.GetLength ()>2) {
			if (csTime.GetAt(2) == ':')  // DBCS checked. substring is supposed to be composed of full single bytes.
				                         //(and BTW ':' is not in the trailing bytes range  (<0x40))
				csExpire = csTemp.Left(index); // remove the time (the warning appeared already if not 00:00)
		}
  }
  if (csExpire.Compare(_T(""))==0)
	  return csExpire;
  index = csExpire.Find(_T('-'));
  CString csOne = csExpire.Left(index);
  csExpire = csExpire.Mid(index+1);
  index = csExpire.Find(_T('-'));
  CString csTwo = csExpire.Left(index);
  csExpire = csExpire.Mid(index+1);
  int year = atoi(LPCTSTR(csExpire));

  CString csReformatted;
  csReformatted.Format(_T("%s %s %d"), (LPCTSTR)csTwo, (LPCTSTR)csOne, year);
  return csReformatted;
}

void CChildFrame::OnButtonExpireDate() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_TABLE);

  CuDlgExpireDate dlg;
  dlg.m_csTableName = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
  dlg.m_csOwnerName = lpRecord->ownerName;

  // Call GetDetailInfo just to pick table info about expiration date
  {
    CWaitCursor hourGlass;

    // Get info on the object
    TABLEPARAMS tableparams;
    memset (&tableparams, 0, sizeof (tableparams));

    x_strcpy ((char *)tableparams.DBName,     (const char *)lpRecord->extra);
    x_strcpy ((char *)tableparams.objectname, RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName)));
    x_strcpy ((char *)tableparams.szSchema,   (const char *)lpRecord->ownerName);
    tableparams.detailSubset = TABLE_SUBSET_NOCOLUMNS;  // minimize query duration

    int dummySesHndl;
    int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(lpDomData->psDomNode->nodeHandle),
                                 OT_TABLE,
                                 &tableparams,
                                 FALSE,
                                 &dummySesHndl);
    if (iResult != RES_SUCCESS)
      return;

    if (tableparams.bExpireDate) {
		dlg.m_iRad = 1;
		dlg.m_csExpireDate = ReformatDate(tableparams.szExpireDate);
		if (dlg.m_csExpireDate.Compare(_T(""))==0) {
			CString rString;
			rString.LoadString(IDS_EXPIREDATE_BADDATEFORMAT);
			if (BfxMessageBox (rString, MB_ICONSTOP|MB_YESNO) != IDYES)
				return;
			dlg.m_csExpireDate.LoadString(IDS_FORMAT_DISPLAY);
		}
		else {
			if (!x_stristr((LPUCHAR)tableparams.szExpireDate,(LPUCHAR)"00:00")) {
				CString rString;
				char buftemp[600]; // AfxFormatString limited to 256 chars !
				rString.LoadString(IDS_EXPIREDATE_TIMEZONEDIFFERENCE);
				wsprintf(buftemp,(char*)(LPCTSTR)rString,tableparams.szExpireDate);
				rString=(LPCTSTR)(buftemp);
				//AfxFormatString1(rString, IDS_EXPIREDATE_TIMEZONEDIFFERENCE, (LPCTSTR)(LPUCHAR)tableparams.szExpireDate);
 				if (BfxMessageBox (rString, MB_ICONSTOP|MB_YESNO) != IDYES)
					return;
			}
		}
    }
    else {
      dlg.m_iRad = 0;
      dlg.m_csExpireDate = _T("");
    }
    // liberate detail structure
    FreeAttachedPointers (&tableparams,  OT_TABLE);
  }

  dlg.m_csVirtNodeName = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
  dlg.m_csParentDbName = lpRecord->extra;
  int iret = dlg.DoModal();

  // Note: we don't care about iret at this time
}

void CChildFrame::OnButtonComment() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_TABLE || CurItemObjType == OT_VIEW);

  CxDlgObjectComment dlg;
  dlg.m_csDBName      = lpRecord->extra;
  dlg.m_csObjectName  = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
  dlg.m_csObjectOwner = lpRecord->ownerName;
  dlg.m_ListColumn    = NULL;
  dlg.m_nNodeHandle   = lpDomData->psDomNode->nodeHandle;
  dlg.m_iCurObjType = CurItemObjType;
  dlg.DoModal();
}

void CChildFrame::OnButtonUsermod()
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  CxDlgUserMod dlg;
  if (CurItemObjType == OT_TABLE)
  {
    dlg.m_csDBName = lpRecord->extra;
    dlg.m_csTableName  = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
    dlg.m_csTableOwner = lpRecord->ownerName;
  }
  else if (CurItemObjType == OT_DATABASE)
  {
    dlg.m_csDBName = lpRecord->objName;
    dlg.m_csTableName  = _T("");
    dlg.m_csTableOwner = _T("");
  }
  else
  {
    dlg.m_csDBName = lpRecord->extra;
    dlg.m_csTableName  = _T("");
    dlg.m_csTableOwner = _T("");
  }
  dlg.m_nNodeHandle = lpDomData->psDomNode->nodeHandle;
  dlg.DoModal();
}

void CChildFrame::OnUpdateButtonExpireDate(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database on pure ingres node
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer!=OIVERS_NOTOI) {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel) {
      LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                    LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->recType == OT_TABLE)
        if (lpRecord->parentDbType == DBTYPE_REGULAR)
          if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName))
            if (!IsSystemObject(lpRecord->recType, lpRecord->objName, lpRecord->ownerName))
              bEnable = TRUE;
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonJournaling() 
{
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);

  DWORD dwCurSel = GetCurSel(lpDomData);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  int CurItemObjType = lpRecord->recType; // Same as GetItemObjType()
  ASSERT(CurItemObjType == OT_TABLE);

  CuDlgTableJournaling dlg;
  dlg.m_csTableName = RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName));
  dlg.m_csOwnerName = lpRecord->ownerName;

  // Call GetDetailInfo just to pick table info about journaling
  {
    CWaitCursor hourGlass;

    // Get info on the object
    TABLEPARAMS tableparams;
    memset (&tableparams, 0, sizeof (tableparams));

    x_strcpy ((char *)tableparams.DBName,     (const char *)lpRecord->extra);
    x_strcpy ((char *)tableparams.objectname, RemoveDisplayQuotesIfAny((const char *)StringWithoutOwner(lpRecord->objName)));
    x_strcpy ((char *)tableparams.szSchema,   (const char *)lpRecord->ownerName);
    tableparams.detailSubset = TABLE_SUBSET_NOCOLUMNS;  // minimize query duration

    int dummySesHndl;
    int iResult = GetDetailInfo ((LPUCHAR)GetVirtNodeName(lpDomData->psDomNode->nodeHandle),
                                 OT_TABLE,
                                 &tableparams,
                                 FALSE,
                                 &dummySesHndl);
    if (iResult != RES_SUCCESS)
      return;

    // prepare dialog input
    dlg.m_cJournaling = tableparams.cJournaling;

    // liberate detail structure
    FreeAttachedPointers (&tableparams,  OT_TABLE);
  }

  dlg.m_csVirtNodeName = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
  dlg.m_csParentDbName = lpRecord->extra;
  int iret = dlg.DoModal();

  // Note: we don't care about iret at this time
}

void CChildFrame::OnUpdateButtonJournaling(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database on pure ingres node
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer!=OIVERS_NOTOI) {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel) {
      LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                    LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->recType == OT_TABLE)
        if (lpRecord->parentDbType == DBTYPE_REGULAR)
          if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName))
            if (!IsSystemObject(lpRecord->recType, lpRecord->objName, lpRecord->ownerName) && 
				(!IsVW() || (IsVW() && getint(lpRecord->szComplim + STEPSMALLOBJ + STEPSMALLOBJ) == 0)))
              bEnable = TRUE;
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);

  // if unicenter driven and maximize doc requested:
  // Must manage once more width percentage if specified,
  // due to MFC Implementation that postpones the MAXIMIZED attribute
  if (IsUnicenterDriven() && nType == SIZE_MAXIMIZED) {
    if (m_pWndSplitter) {   // not too early!
      CuCmdLineParse* pCmdLineParse = GetAutomatedGeneralDescriptor();
      ASSERT (pCmdLineParse);
      if (pCmdLineParse->DoWeMaximizeWin()) {
        CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
        ASSERT (pDescriptor);
        ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
        if (pDescriptor->HasSplitbarPercentage()) {
          int cxCur, cxMin;

          // set left pane width - let right pane freewheelin'
          int pct = pDescriptor->GetSplitbarPercentage();
          int idealWidthPane0 = (long)cx * (long)pct / 100L;
          m_pWndSplitter->GetColumnInfo(0, cxCur, cxMin);
          m_pWndSplitter->SetColumnInfo(0, idealWidthPane0, cxMin);

          m_pWndSplitter->RecalcLayout();
        }
      }
    }
  }
}

// called from dom.c
extern "C" BOOL AutomatedDomDocWithRootItem()
{
  if (IsUnicenterDriven()) {
    CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
    ASSERT (pDescriptor);
    ASSERT (pDescriptor->GetType() == TYPEDOC_DOM);
    if (pDescriptor->HasSingleTreeItem()) {
      ASSERT (pDescriptor->HasObjDesc());
      if (pDescriptor->HasObjDesc())
        return TRUE;
    }
  }
  return FALSE;
}

extern "C" BOOL CreateAutomatedDomRootItem(LPDOMDATA lpDomData)
{
  ASSERT (lpDomData);
  ASSERT (AutomatedDomDocWithRootItem());
  CuWindowDesc* pDesc = GetAutomatedWindowDescriptor();
  ASSERT (pDesc);
  if (!pDesc)
    return FALSE;
  CuDomObjDesc* pObjDesc = pDesc->GetDomObjDescriptor();
  ASSERT (pObjDesc);
  if (!pObjDesc)
    return FALSE;
  if (!pObjDesc->CheckLowLevel(lpDomData->psDomNode->nodeHandle))
    return FALSE;
  if (!pObjDesc->CreateRootTreeItem(lpDomData))
    return FALSE;

  // Success speaks for itself
  return TRUE;
}

void CChildFrame::OnClose() 
{
	if (!theApp.CanCloseContextDrivenFrame())
		return;
	CMDIChildWnd::OnClose();
}

//
// Update the Icon image in the header of the DOM Right pane:
void TS_DOMRightPaneUpdateBitmap(HWND hwndMdi, LPVOID lpDomData, DWORD dwCurSel, LPVOID lpRecord)
{
	LPDOMDATA lpData = (LPDOMDATA)lpDomData;
	CWnd *pWnd;
	pWnd = CWnd::FromHandlePermanent(hwndMdi);
        return;
	if (!pWnd)
		return;
	try 
	{
		CMainMfcView *pView = (CMainMfcView*)pWnd;
		CMainMfcDoc *pMainMfcDoc = (CMainMfcDoc *)pView->GetDocument();
		if (pMainMfcDoc)
		{
			CuDlgDomTabCtrl* pTabDlg  = pMainMfcDoc->GetTabDialog();
			//
			//  Get the Image (Icon) of the current selected Tree Item:
			HICON hIcon = CChildFrame::DOMTreeGetCurrentIcon(lpData->hwndTreeLb);
			pTabDlg->UpdateBitmapTitle(hIcon);
		}
	}

	catch(CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	catch(CResourceException* e)
	{
		BfxMessageBox (VDBA_MfcResourceString(IDS_E_LOAD_DLG));//"Cannot load dialog box"
		e->Delete();
	}
	catch(...)
	{
		CString strMsg;
		strMsg.LoadString(IDS_E_CONSTRUCT_PROPERTY);
		BfxMessageBox (strMsg);
	}
}

void CChildFrame::OnUpdateButtonComment(CCmdUI* pCmdUI) 
{
  // Enable if non-distributed database on pure ingres node
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer!=OIVERS_NOTOI) {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel) {
      LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                    LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->recType == OT_TABLE || lpRecord->recType == OT_VIEW)
        if (lpRecord->parentDbType == DBTYPE_REGULAR)
          if (!IsNoItem(lpRecord->recType, (char*)lpRecord->objName))
            if (!IsSystemObject(lpRecord->recType, lpRecord->objName, lpRecord->ownerName))
              bEnable = TRUE;
    }
  }

  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnUpdateButtonUsermod(CCmdUI* pCmdUI) 
{
  BOOL bEnable = FALSE;
  LPDOMDATA lpDomData = GetLPDomData(this);
  ASSERT (lpDomData);
  if (lpDomData->ingresVer>=OIVERS_26)
  {
    DWORD dwCurSel = GetCurSel(lpDomData);
    if (dwCurSel)
    {
    LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if ( HasTrueParentDB(lpDomData, NULL))
      {
        if (lpRecord != 0 && 
					  (!IsSystemObject(lpRecord->recType, lpRecord->objName, lpRecord->ownerName)||lpRecord->parentDbType ==DBTYPE_COORDINATOR)
					 )
        {
          if (lpRecord->parentDbType != DBTYPE_DISTRIBUTED)
            bEnable = TRUE;
        }
	  }

	  if (!CanObjectExistInVectorWise(lpRecord->recType))
		  bEnable=FALSE;
    }
  }
  pCmdUI->Enable(bEnable);
}

void CChildFrame::OnUpdateButtonExport(CCmdUI* pCmdUI) 
{
	BOOL bEnable = TRUE;

	LPDOMDATA lpDomData = GetLPDomData(this);
	DWORD dwCurSel = GetCurSel(lpDomData);
	int   CurItemObjType = GetUnsolvedItemObjType(lpDomData, dwCurSel);
	LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

	// Export : only if item is a database or a table
	switch (CurItemObjType) 
	{
	case OT_DATABASE:
	case OT_TABLE:
		if (lpRecord==0 || IsNoItem(CurItemObjType, (char *)lpRecord->objName)) 
			bEnable = FALSE;
		else 
			bEnable = TRUE;
		break;
	default:
		bEnable = FALSE;
		break;
	}
	// Restricted features if gateway
	if (lpDomData->ingresVer==OIVERS_NOTOI) 
	{
		if (CurItemObjType == OT_STATIC_DATABASE || CurItemObjType == OT_DATABASE)
			bEnable = FALSE;
	}

	// Post correction for STAR DDB
	if (lpRecord != 0 && lpRecord->parentDbType == DBTYPE_DISTRIBUTED)
		bEnable = FALSE;

	pCmdUI->Enable(bEnable);
}

void CChildFrame::OnButtonExport() 
{
	RelayCommand(ID_BUTTON_EXPORT);
}
