#define WINSTART_INCLUDED
/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Visual DBA
**
**    Source : dom.c
**    manage a DOM document
**        splitted in domsplit.c May 5, 95
**
**    Author : Emmanuel Blattes
**
**  History after 04-May-1999:
**
**    31-May-1999 (schph01)
**      bug # 97167
**      VDBA / Dom window / Replication / CDDS / Activate replication :
**      when accepting the "all full peer databases" option, the
**      operation is done more than once for some of the databases
**      resulting in the remote command dialog box to appear more than
**      once on the same database
**
**    31-May-1999 (schph01)
**      bug # 97168:
**      VDBA / Dom window / Replication / CDDS / Activate replication:
**      when accepting the "all full peer databases" option, the operation fails for
**      databases that are not owned by the user defined in the node definition
**
**    08-Sep-1999 (schph01)
**      bug # 98681:
**      VDBA / DOM / Replication : the "activate", "deactivate", and "create keys"
**      menu option doesn't work against UNIX servers ,because the corresponding
**      repcfg command is sent to RMCMD in uppercase which works under NT but
**      not under UNIX.
**
**    uk$so01 (20-Jan-2000, Bug #100063)
**         Eliminate the undesired compiler's warning
**  20-May-2000 (noifr01)
**     (bug 99242) clean-up for DBCS compliance
**  16-Nov-2000 (schph01)
**     SIR 102821 add management for new menu "Comment"
**  22-Dec-2000 (noifr01)
**   (SIR 103548) updated the way to get the application title
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  11-Apr-2001 (schph01)
**    (sir 103292) add management for new menu 'usermod'.
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
**  26-Mar-2002 (noifr01)
**   (bug 107135) removed the "force refresh" dialog box in DOM windows: force
**    refresh now refreshes all data
**  18-Mar-2003 (schph01)
**   sir 107523 Add Branches 'Sequence' and 'Grantees Sequence'
**  09-Apr-2003 (noifr01)
**   (sir 107523) cleanup when reviewing refresh features for managing
**   sequences
**  03-Feb-2004 (uk$so01)
**     SIR #106648, Split vdba into the small component ActiveX or COM:
**     Integrate Export assistant into vdba
**  10-Nov2004 (schph01)
**     (bug 113426) Update the branch ICE before a force refresh
**  23-Mar-2010 (drivi01)
**     In case if we have multiple DOM windows open, when the Window
**     is activated, update the VectorWise bit in LPDOMDATA
**     structure with 1 if it's VectorWise window or 0 if it isn't.
**     Make sure to keep the static variable in dbaginfo.sc
**     with accurate value, but setting it on Window Activation
**     and Window creation.
**  12-May-2010 (drivi01)
**     For newly added "Create Index" menu item, added
**     case statement to handle the selection.
**  30-Jun-2010 (drivi01)
**     ICE is being removed from release 10.0.
**     Remove ICE from the DOM tree.
*****************************************************************************/

// ---   IMPORTANT NOTES !!!    ---
// Every MDI document managed by this application MUST
// have a window extra bytes count of 4 at least, and the 4 first
// extra bytes MUST point on a structure with the FIRST member as
// an int containing TYPEDOC_XXX according to the document type.
// (constants to be defined in main.h)
//
// See also the functions CommandForMDIDoc and QueryDocType in main.c
//
//
// ALSO : in the dll that manages the nanbar, the CreateWindowEx of the
// nanbar MUST be done with WS_CLIPSIBLINGS. (function CreateNanBarWnd
// in source nanbtn.c in directory nanact). Otherwise, the special controls
// on the nanbar might suprisingly disappear...
//

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
#include "dbafile.h"
#include "dom.h"
#include "resource.h"
#include "winutils.h"
#include "treelb.e"   // tree list dll
#include "nanact.e"   // nanbar and status bar dll
#include "dbadlg1.h"  // dialog boxes part one ("find" box constants)
// for print functions in print.c
#include "typedefs.h"
#include "stddlg.e"
#include "print.e"
#include "getdata.h"        // dialog boxes part alterdb //PS
#include "msghandl.h"
#include "monitor.h"
#include "dll.h"
#include "dgerrh.h" // MessageWithHistoryButton

#include "replutil.h"

#include "prodname.h"

//
// local structures definition
//
typedef struct  _sDomCreateData
{
  int       domCreateMode;                // dom creation mode: tearout,...
  HWND      hwndMdiRef;                   // referred mdi dom doc
  FILEIDENT idFile;                       // if loading: file id to read from
  LPDOMNODE lpsDomNode;                   // node created from previous name
} DOMCREATEDATA, FAR *LPDOMCREATEDATA;

//
// for tree status bar functions
//
#include "statbar.h"
#include "esltools.h"

#define RB_DOM_NFIELDS    7    // Number of controls on tree status bar

#define RB_SMALL_GAP      2    // Space between text and field.
#define RB_LARGE_GAP      6    // Space between field and next fields text.
#define RB_LEFT_MARGIN    4    // Gap left of first fields text.
#define RB_TOP_MARGIN     2
#define RB_BOTTOM_MARGIN  2
#define RB_VERT_SPACER    2

typedef struct tagDOMTREESTATUSBAR
{
  int nID;
  int nStringId;
  WORD wType;
  int nSysTextColor;
  int nSysFaceColor;
  WORD wFormat;
  WORD wLength;
  int nGap;    // Gap following field.
}DOMTREESTATUSBAR, FAR * LPDOMTREESTATUSBAR;


// defined in main.c - MAINLIB
extern HWND GetVdbaDocumentHandle(HWND hwndDoc);

// defined in mainfrm.cpp
extern HWND MainFrmCreateDom();
extern void MfcDocSetCaption(HWND hWnd, LPCSTR lpszTitle);
extern BOOL MfcDelayedUpdatesAccepted();
extern void MfcDelayUpdateOnNode(char *nodeName);

// defined in ChildFrm.cpp
extern HWND GetBaseOwnerComboHandle(HWND hWnd);
extern HWND GetOtherOwnerComboHandle(HWND hWnd);
extern HWND GetSystemCheckboxHandle(HWND hWnd);
extern void MfcBlockUpdateRightPane();
extern void MfcAuthorizeUpdateRightPane();
extern BOOL MfcFastload(HWND hwndDoc);
extern BOOL MfcInfodb(HWND hwndDoc);
extern BOOL MfcDuplicateDb(HWND hwndDoc);
extern BOOL MfcSecurityAudit(HWND hwndDoc);
extern BOOL MfcExpireDate(HWND hwndDoc);
extern BOOL MfcComment(HWND hwndDoc);
extern BOOL MfcUsermod(HWND hwndDoc);
extern BOOL MfcJournaling(HWND hwndDoc);

// Notify rightpane for refresh - defined in childfrm.cpp
extern void TS_MfcUpdateRightPane(HWND hwndDoc, LPDOMDATA lpDomData, BOOL bCurSelStatic, DWORD dwCurSel, int CurItemObjType, LPTREERECORD lpRecord, BOOL bHasProperties, BOOL bUpdate, int initialSel, BOOL bClear);

// define in buildsvr.cpp
extern void MfcDlgBuildSvr(long CurNodeHandle, char *CurNode,char *CurDBName,char *CurUser);

// defined in DgArccln.cpp
extern int MfcDlgArcClean(char*  pBufDate,
                          int    iLen);

// defined in replikey.cpp
extern int MfcReplicationSelectKeyOption(char *pTblName);

// defined in childfrm.cpp
extern BOOL AutomatedDomDocWithRootItem();
extern BOOL CreateAutomatedDomRootItem(LPDOMDATA lpDomData);

// defined in domdisp.c
extern int UpdateIceDisplayBranch( HWND hwndMdi, LPDOMDATA lpDomData);

// defined here
BOOL DomIsSystemChecked(HWND hWnd);
static void SubclassCombo(HWND hwndCombo, LPDOMDATA lpDomData);
static void SubclassCheckbox(HWND hwndCheck, LPDOMDATA lpDomData);
static void UnSubclassAllToolbarControls(LPDOMDATA lpDomData);


static HFONT NEAR MakeTreeStatusBarFont(HWND hwnd);
static void NEAR SizeTreeStatusBarWnd(HWND hwndParent);
static int  NEAR GetTreeStatusBarHeight(HWND hwnd);
//
//  End of section for tree status bar functions
//

//
// global variables
//
#ifdef ISQL_WIN
char *szDomMDIChild = "ingres_isql_mdi_dom";  //Class name for dom MDI window
#else
char *szDomMDIChild = "ingres_dbatool_mdi_dom";  //Class name for dom MDI window
#endif

// sequential numbers for doms captions
int seqDomNormal;      // normal dom sequential number
int seqDomTearOut;     // teared out dom sequential number
int seqDomRepos;       // repositioned dom sequential number
int seqDomScratchpad;  // scratchpad dom sequential number

//
// local constants
//
#define CBWNDEXTRA_DOM  sizeof(char *)   // extra bytes for DOM class

//
// static functions definition
//
static BOOL NEAR DomWindowCreate(HWND hwndMdi, LPCREATESTRUCT lpCs);
static BOOL NEAR DomWindowCommand(HWND hwndMdi, LPDOMDATA lpDomData, int id, HWND hwndCtl, UINT codeNotify);
static VOID NEAR DomWindowFind(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);
static VOID NEAR DomWindowSize(HWND hwndMdi, LPDOMDATA lpDomData, UINT state, int cx, int cy);
static BOOL NEAR DomFillNanBar(HWND hwndMdi, LPDOMDATA lpDomData);
static int  NEAR GetControlsMaxHeight(LPDOMDATA lpDomData);
static BOOL NEAR DomFillTree(LPDOMDATA lpDomData, int domCreateMode, HWND hwndMdiRef, FILEIDENT idFile);
static BOOL NEAR RestartFromPosition(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR UpdateShowSystemMenuItem(LPDOMDATA lpDomData);
static VOID NEAR InstallReplicatorTables(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorPropagate(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorBuildServer(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorReconcile(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorDereplicate(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorMobile(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorArcclean(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorRepmod(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorCreateKeys(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorActivate(HWND hwndMdi, LPDOMDATA lpDomData);
static VOID NEAR ReplicatorDeactivate(HWND hwndMdi, LPDOMDATA lpDomData);

static VOID NEAR ManageFiltersBox(HWND hwndMdi, LPDOMDATA lpDomData);

static VOID NEAR DomWindowChangePreferences(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);
static BOOL NEAR ManageLocate(HWND hwndMdi, LPDOMDATA lpDomData);
static BOOL  NEAR RefreshTrees(HWND hwndMdi, LPDOMDATA lpDomData);

// special objects functions
static BOOL NEAR ManageNotifMsg(HWND hwndMdi, LPDOMDATA lpDomData, int id, HWND hwndCtl, UINT codeNotify);
// UK-CLEAL static HWND NEAR CreateCombo(LPDOMDATA lpDomData, HWND hwndParent, HWND hwndBar, UINT helpStringId, WORD idCommand);
static BOOL NEAR FillCombo(LPDOMDATA lpDomData, HWND hwndCombo, BOOL bForce);
static VOID NEAR SetComboSelection(HWND hwndCombo, UCHAR *filter);
static VOID NEAR SetCheckboxState(HWND hwndCheck, BOOL checked);
static VOID NEAR DeleteNanBarSpecialObjects(LPDOMDATA lpDomData);

static BOOL FilterOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void FilterOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void FilterOnDestroy (HWND hwnd);

BOOL DomOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL DomOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);

static BOOL DomOnEraseBkgnd(HWND hwnd, HDC hdc);
static void DomOnPaint(HWND hwnd);
static void DomOnSize(HWND hwnd, UINT state, int cx, int cy);
static HBRUSH DomOnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type);
static void DomOnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);
void        DomOnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
static void DomOnSetFocus(HWND hwnd, HWND hwndOldFocus);
static void DomOnDestroy(HWND hwnd);
void        DomOnMdiActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate);

LRESULT WINAPI EXPORT TreeSubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CommonTreeSubclassDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void TreeOnChar(HWND hwnd, TCHAR ch, int cRepeat);

// Special for menu management in mainlib
extern void SetReplicatorRequestMandatory(HWND hWnd, LPDOMDATA lpDomData);   // domsplit.c


//
//  DomRegisterClass(VOID)
//
BOOL DomRegisterClass(VOID)
{
  WNDCLASS  wc;

  wc.style         = 0;
  wc.lpfnWndProc   = DomMDIChildWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = CBWNDEXTRA_DOM;
  wc.hInstance     = hInst;
  wc.hIcon         = LoadIcon(hResource, MAKEINTRESOURCE(IDI_DOC_DOM));
  wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szDomMDIChild;

  if (!RegisterClass (&wc))
    return FALSE;
  else
    return TRUE;
}

//
//  build the caption of the dom according to it's create mode and other
//
static VOID NEAR  MakeDomCaption(char *sz, char *VirtNodeName, int domCreateMode, HWND hwndMdiRef, BOOL bIncrementNum)
{
  LPDOMDATA lpRefDomData;   // domdata of the referred mdi doc
  char      rsCapt[BUFSIZE];
  char      rsType[BUFSIZE];
  UINT      rid;
  int       num;

  // info on the dom tree line object
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  DWORD         dwItem;
  LPTREERECORD  lpRecord;
  char          rsSubType[BUFSIZE];

  // 17/07/97: unix porting does not accept GetWindowLong on null hwnd
  if (hwndMdiRef)
    lpRefDomData = (LPDOMDATA)GetWindowLong(hwndMdiRef, 0);
  else
    lpRefDomData = NULL;

  LoadString (hResource, IDS_DOMDOC_TITLE, rsCapt, sizeof(rsCapt));
  switch (domCreateMode) {
    case DOM_CREATE_QUERYNODE:
      rid = IDS_DOMDOC_TITLE_NORMAL;
      if (bIncrementNum)
        ++seqDomNormal;
      num = seqDomNormal;
      break;
    case DOM_CREATE_TEAROUT:
      rid = IDS_DOMDOC_TITLE_TEAROUT;
      if (bIncrementNum)
        ++seqDomTearOut;
      num = seqDomTearOut;
      break;
    case DOM_CREATE_SCRATCHPAD:
      rid = IDS_DOMDOC_TITLE_SCRATCHPAD;
      if (bIncrementNum)
        ++seqDomScratchpad;
      num = seqDomScratchpad;
      break;
    case DOM_CREATE_REPOSITION:
    case DOM_CREATE_NEWWINDOW:
      // take rid and number according to cloned dom type
      switch (lpRefDomData->domType) {
        case DOM_TYPE_NORMAL:
          rid = IDS_DOMDOC_TITLE_NORMAL;
          if (bIncrementNum)
            ++seqDomNormal;
          num = seqDomNormal;
          break;
        case DOM_TYPE_TEAROUT:
          rid = IDS_DOMDOC_TITLE_TEAROUT;
          if (bIncrementNum)
            ++seqDomTearOut;
          num = seqDomTearOut;
          break;
        case DOM_TYPE_SCRATCHPAD:
          rid = IDS_DOMDOC_TITLE_SCRATCHPAD;
          if (bIncrementNum)
            ++seqDomScratchpad;
          num = seqDomScratchpad;
          break;
        case DOM_TYPE_REPOS:
          rid = IDS_DOMDOC_TITLE_REPOS;
          if (bIncrementNum)
            ++seqDomRepos;
          num = seqDomRepos;
          break;
      }
      break;
  }
  LoadString(hResource, rid, rsType, sizeof(rsType));
  if (rid==IDS_DOMDOC_TITLE_NORMAL || rid==IDS_DOMDOC_TITLE_SCRATCHPAD)
    wsprintf(sz, rsCapt, VirtNodeName, rsType, num);
  else {
    // get object parenthood
    dwItem = GetCurSel(lpRefDomData);
    lpRecord = (LPTREERECORD) SendMessage(lpRefDomData->hwndTreeLb,
                              LM_GETITEMDATA, 0, (LPARAM)dwItem);
    if (IsItemObjStatic(lpRefDomData, dwItem)) {
      if (lpRecord->extra3[0] != '\0')
        wsprintf(buf, "[%s][%s][%s] %s", lpRecord->extra,
                                         lpRecord->extra2,
                                         lpRecord->extra3,
                                         lpRecord->objName);
      else if (lpRecord->extra2[0] != '\0')
        wsprintf(buf, "[%s][%s] %s", lpRecord->extra,
                                     lpRecord->extra2,
                                     lpRecord->objName);
      else if (lpRecord->extra[0] != '\0')
        wsprintf(buf, "[%s] %s", lpRecord->extra,
                                 lpRecord->objName);
      else
        wsprintf(buf, "%s", lpRecord->objName);
    }
    else {
      LoadString(hResource, GetObjectTypeStringId(lpRecord->recType),
                        rsSubType, sizeof(rsSubType));
      if (lpRecord->extra3[0] != '\0')
        wsprintf(buf, "%s : (%s)(%s)(%s) %s", rsSubType,
                                         lpRecord->extra,
                                         lpRecord->extra2,
                                         lpRecord->extra3,
                                         lpRecord->objName);
      else if (lpRecord->extra2[0] != '\0')
        wsprintf(buf, "%s : (%s)(%s) %s", rsSubType,
                                     lpRecord->extra,
                                     lpRecord->extra2,
                                     lpRecord->objName);
      else if (lpRecord->extra[0] != '\0')
        wsprintf(buf, "%s : (%s) %s", rsSubType,
                                 lpRecord->extra,
                                 lpRecord->objName);
      else
        wsprintf(buf, "%s : %s", rsSubType, lpRecord->objName);
    }
    wsprintf(buf2, rsType, buf);
    wsprintf(sz, rsCapt, VirtNodeName, buf2, num);
  }
}

DOMCREATEDATA globDomCreateData;
//
// MakeNewDomMDIChild
//
//  Creates a new MDI document with dom style
//
//  returns the MDI doc. handle, or NULL if error
HWND MakeNewDomMDIChild(int domCreateMode, HWND hwndMdiRef, FILEIDENT idFile, DWORD dwData)
{
  HWND            hwnd = NULL;
  MDICREATESTRUCT mcs;
  LPDOMNODE       lpsDomNode;
  UCHAR           VirtNodeName[MAXOBJECTNAME];
  char            sz[BUFSIZE];
  DOMCREATEDATA   domCreateData;  // temporary struct for dom creation

  // referred doc structures
  LPDOMDATA       lpRefDomData;

  // variables for loading
  int         retval;
  COMMONINFO  loadDocInfo;

  gMemErrFlag = MEMERR_NOMESSAGE;

  lpsDomNode = (LPDOMNODE) ESL_AllocMem(sizeof(DOMNODE));
  if (lpsDomNode == 0) {
    MessageBox(GetFocus(),
               ResourceString(IDS_E_MEMERR_NEWDOM),
               NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    gMemErrFlag = MEMERR_STANDARD;
    return NULL;
  }

  if (domCreateMode == DOM_CREATE_LOAD) {

    // Step 2.1: Load Size/Pos/Iconstate/DOMCaption,
    // and fill the create structure reading from the file
    retval = DBAReadData(idFile, (void *)(LPCOMMONINFO)&loadDocInfo,
                                 sizeof(COMMONINFO));
    if (retval != RES_SUCCESS)
      return 0;
    mcs.szTitle = loadDocInfo.szTitle;
    x_strcpy(sz, loadDocInfo.szTitle);
    mcs.szClass = szDomMDIChild;
    mcs.hOwner  = hInst;
    mcs.x       = loadDocInfo.x;
    mcs.y       = loadDocInfo.y;
    mcs.cx      = loadDocInfo.cx;
    mcs.cy      = loadDocInfo.cy;
    mcs.style   = 0L;
    if (loadDocInfo.bIcon)
      mcs.style = WS_MINIMIZE;
    // fix 18/11/96 : delayed maximize after doc creation, due to menu problems
    //if (loadDocInfo.bZoom)
    //  mcs.style = WS_MAXIMIZE;
    mcs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetOIVers(loadDocInfo.ingresVer);
  }
  else {
    switch (domCreateMode) {
      case DOM_CREATE_QUERYNODE:
      case DOM_CREATE_SCRATCHPAD:
        x_strcpy(VirtNodeName, (LPSTR)dwData);
        if (!CheckConnection(VirtNodeName,FALSE,TRUE)) {
          ESL_FreeMem(lpsDomNode);
          return NULL;
        }
        break;
      default:
        lpRefDomData = (LPDOMDATA)GetWindowLong(hwndMdiRef, 0);
        lstrcpy(VirtNodeName,
                GetVirtNodeName(lpRefDomData->psDomNode->nodeHandle));
        break;
    }

    // Connexion to the database
    lpsDomNode->nodeHandle = OpenNodeStruct(VirtNodeName);
    if (lpsDomNode->nodeHandle == -1) {
      //"Maximum number of connections has been reached - Cannot create DOM Window"
      MessageBox(GetFocus(),ResourceString(IDS_ERR_CREATE_DOM),
                 NULL, MB_OK | MB_ICONEXCLAMATION);
      ESL_FreeMem(lpsDomNode);
      return NULL;
    }

    // Fill the create structure
    MakeDomCaption(sz, VirtNodeName, domCreateMode, hwndMdiRef, TRUE);

  }

  domCreateData.domCreateMode = domCreateMode;
  domCreateData.hwndMdiRef    = hwndMdiRef;
  domCreateData.lpsDomNode    = lpsDomNode;
  domCreateData.idFile        = idFile;

  memcpy(&globDomCreateData, &domCreateData, sizeof(domCreateData));
  hwnd = MainFrmCreateDom();
  if (hwnd) {
    MfcDocSetCaption(hwnd, sz);
	/* MASQUED OUT : CAUSES TOOLBAR PROBLEM - MANAGED ON C++ SIDE:
    // adjust size/pos/maximized/minimized of mdidoc,
    // which is the parent of hwnd
    if (domCreateMode == DOM_CREATE_LOAD) {
      if (loadDocInfo.bIcon)
        ShowWindow(GetParent(hwnd), SW_SHOWMINIMIZED);
      else if (loadDocInfo.bZoom)
        ShowWindow(GetParent(hwnd), SW_SHOWMAXIMIZED);
      else
        SetWindowPos(GetParent(hwnd), HWND_TOP,
                     loadDocInfo.x, loadDocInfo.y,
                     loadDocInfo.cx, loadDocInfo.cy,
                     SWP_NOZORDER);
    }
	*/
  }
  return hwnd;
}


//
// DomMDIChildWndProc ( hwnd, wMsg, wParam, lParam )
//
//  window proc for the dom.
//
// See Note from Microsoft :
//
//  NOTE: If cases are added for WM_CHILDACTIVATE, WM_GETMINMAXINFO,
//    WM_MENUCHAR, WM_MOVE, WM_NEXTMENU, WM_SETFOCUS, WM_SIZE,
//    or WM_SYSCOMMAND, these messages should be passed on
//    to DefMDIChildProc even if we handle them. See the SDK
//    Reference entry for DefMDIChildProc
//
LRESULT CALLBACK __export DomMDIChildWndProc (HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  LPDOMDATA lpDomData;
  BOOL      bProcessed = TRUE;
  LONG      lRet;
  char      rsBuf[BUFSIZE];

  // Critical section reentrance problem,
  // combined with IIPROMPTALL connect problem
  if (InSqlCriticalSection()) {
    if (!InCriticalConnectSection()) {
      // Not in critical connect section: self-repost
      // Aliteration by Emb Oct. 2, 95:
      // except for LM_QUERYICONS, which NEEDS synchronous management
      // (lParam contains pointer on dll automatic variable), and whose
      // synchronous management does not lead to reentrance crash.
      if (wMsg != LM_QUERYICONS) {
        PostMessage(hwnd, wMsg, wParam, lParam);
        return 0L;
      }
    }
    // Special management Fnn/Emb 14/11/95 :
    // delay this message after the critical connect section
    if (InCriticalConnectSection()) {
      if (wMsg == LM_NOTIFY_EXPAND) {
        StoreMessage(hwnd, wMsg, wParam, lParam);
        return 0L;
      }
    }
  }

  // First of all : get lpDomData
  lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

  // process notification messages from the tree
  lRet = 0L;    // default value
  bProcessed = DomTreeNotifMsg(hwnd, wMsg, lpDomData, wParam, lParam, &lRet);
  if (bProcessed) {
    switch (wMsg) {
      case LM_NOTIFY_ENDDRAGDROP:
      //case LM_NOTIFY_SELCHANGE:
      case LM_NOTIFY_EXPAND:
      case LM_NOTIFY_LBUTTONDBLCLK:
        bSaveRecommended = TRUE;
        break;
    }
    return lRet;    // Processed, lRet may contain data (LM_QUERYICONS)
  }

  if (lpDomData && wMsg == lpDomData->uNanNotifyMsg)
  {
     switch (wParam)
     {
        case NANBAR_NOTIFY_MSG:
        {
            int nStringId = (int)LOWORD(lParam);
            // the icon bar notifies us for displaying a text in the status bar
            if (lpDomData->hwndStatus) 
            {
                rsBuf[0] = '\0';
                LoadString(hResource, nStringId, rsBuf, sizeof(rsBuf));
                Status_SetMsg(lpDomData->hwndStatus, rsBuf);
                lpDomData->bTreeStatus = FALSE;
                if (lpDomData->bShowStatus)
                    BringWindowToTop(lpDomData->hwndStatus);

                // clear the global status bar message, except if refresh status
                if (!ESL_IsRefreshStatusBarOpened())
                {
                    if (StatusBarPref.bVisible) 
                    {
                        rsBuf[0] = '\0';
                        Status_SetMsg(hwndStatusBar, rsBuf);
                    }
                }
            }
            break;
        }
     }

     return 0;
  }

  switch (wMsg) 
  {
    HANDLE_MSG(hwnd, WM_CREATE,     DomOnCreate);
    HANDLE_MSG(hwnd, WM_SIZE,       DomOnSize);
    HANDLE_MSG(hwnd, WM_DESTROY,    DomOnDestroy);
    HANDLE_MSG(hwnd, WM_SETFOCUS,   DomOnSetFocus);
    BOOL_HANDLE_WM_COMMAND(hwnd, wParam, lParam, DomOnCommand);
#ifdef WIN32
    HANDLE_MSG(hwnd, WM_CTLCOLORBTN,     DomOnCtlColor);
    HANDLE_MSG(hwnd, WM_CTLCOLORLISTBOX, DomOnCtlColor);
#else
    HANDLE_MSG(hwnd, WM_CTLCOLOR, DomOnCtlColor);
#endif
    HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, DomOnNCLButtonDown);
//    HANDLE_MSG(hwnd, WM_SYSCOMMAND,    DomOnSysCommand);
    HANDLE_MSG(hwnd, WM_MDIACTIVATE,   DomOnMdiActivate);

    case WM_USERMSG_FIND:
      DomWindowFind(hwnd, lpDomData, wParam, lParam);
      break;

    case WM_USER_CHANGE_PREFERENCES:
      DomWindowChangePreferences(hwnd, lpDomData, wParam, lParam);
      break;
  }

  return DefMDIChildProc (hwnd, wMsg, wParam, lParam);
}

BOOL DomOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);
  BOOL  retval;

  lpDomData->delayResequence++;
  retval = DomWindowCommand(hwnd, lpDomData, id, hwndCtl, codeNotify);
  lpDomData->delayResequence--;
  return retval;
}

BOOL DomOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{

    if (!DomWindowCreate(hwnd, lpCreateStruct)) 
    {
        // Masqued 17/10/95 for "memory shortage" error global management
        //char rsBuf[BUFSIZE];
        //LoadString(hResource, IDS_DOMDOC_CANNOTCREATE, rsBuf, sizeof(rsBuf));
        //MessageBox(GetFocus(), rsBuf, NULL,
        //           MB_ICONHAND | MB_OK | MB_TASKMODAL);
        return FALSE;
    }

    bSaveRecommended = TRUE;
    return TRUE;
}

static BOOL DomOnEraseBkgnd(HWND hwnd, HDC hdc)
{
    return FORWARD_WM_ERASEBKGND(hwnd, hdc, DefMDIChildProc);
}

static void DomOnPaint(HWND hwnd)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

    // take here the opportunity to update the menu
    // so the toolbar will not flash (we sincerely hope!)

    if (lpDomData)
    {
        if (lpDomData->bCreatePhase) 
        {
            lpDomData->bCreatePhase = FALSE;
            DomUpdateMenu(hwnd, lpDomData, TRUE);
        }
    }

    FORWARD_WM_PAINT(hwnd, DefMDIChildProc);
}

static void DomOnSize(HWND hwnd, UINT state, int cx, int cy)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

    if (lpDomData->bCreatePhase) {
      lpDomData->bCreatePhase = FALSE;
      DomUpdateMenu(hwnd, lpDomData, TRUE);
    }

    DomWindowSize(hwnd, lpDomData, state, cx, cy);
    
    FORWARD_WM_SIZE(hwnd, state, cx, cy, DefMDIChildProc);
}

static HBRUSH DomOnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
    switch (type) 
    {
        case CTLCOLOR_BTN:        // Button or checkbox
            SetBkColor(hdc, RGB(191, 191, 191));
            return (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    
        case CTLCOLOR_LISTBOX:    // Comboboxes
            SetBkColor(hdc, RGB(191, 191, 191));
            return (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    }

    return (HBRUSH)NULL;
}

static void DomOnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

    // Set back focus on tree so we close potentially opened combo box
    if (lpDomData->hwndTreeLb)
        SetFocus(lpDomData->hwndTreeLb);

    FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, DefMDIChildProc);
}

void DomOnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    // May 14: don't change the menu for mfc dom

}

static void DomOnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

    // Give keyboard entry to the Tree so we have up/down arrows, etc...
    if (lpDomData->hwndTreeLb) 
    {
        SetFocus(lpDomData->hwndTreeLb);
        // update tree status bar immediately
        if (lpDomData->bShowStatus) 
        {
            lpDomData->bTreeStatus = TRUE;
            BringWindowToTop(lpDomData->hwndTreeStatus);
            SendMessage(hwnd, LM_NOTIFY_ONITEM,
                        (WPARAM)lpDomData->hwndTreeLb,
                        (LPARAM)GetCurSel(lpDomData));
        }
    }

    // update the menu checkmark - added Emb Sep. 8, 95
    UpdateShowSystemMenuItem(lpDomData);

  FORWARD_WM_SETFOCUS(hwnd, hwndOldFocus, DefMDIChildProc);
}

static void DomOnDestroy(HWND hwnd)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);
    
    // Manage dom create error case
    if (!lpDomData)
      return;
    
    // Free the tree subclass
    SubclassWindow(lpDomData->hwndTreeLb, lpDomData->lpOldTreeProc);
    FreeProcInstance(lpDomData->lpNewTreeProc);

    UnSubclassAllToolbarControls(lpDomData);
    {
      // request for refresh list of opened windows
      char *nodeName = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
      if (MfcDelayedUpdatesAccepted())
        MfcDelayUpdateOnNode(nodeName);
    }
    TreeDeleteAllRecords(lpDomData);
    CloseNodeStruct(lpDomData->psDomNode->nodeHandle, TRUE);
    ESL_FreeMem (lpDomData->psDomNode);
    ESL_FreeMem (lpDomData);
    SetWindowLong(hwnd, 0, 0L);   // Pure fix May 7, 97: lpDomData not valid anymore

    // NOTE: we don't delete hTreeStatusFont
    // since it was created at application startup
    // and will be deleted at app termination time
    // Next code to be unmasqued if previous comment not true anymore
    //if (IsGDIObject(lpDomData->hTreeStatusFont))
    //  DeleteObject(lpDomData->hTreeStatusFont);

    FORWARD_WM_DESTROY(hwnd, DefMDIChildProc);
}

void DomOnMdiActivate(HWND hwnd, BOOL fActive, HWND hwndActivate, HWND hwndDeactivate)
{
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(hwnd, 0);

  // update gateway type and ingres version
  if (fActive) {
    SetOIVers(lpDomData->ingresVer);
    SetVW(lpDomData->bIsVectorWise);
  }
  FORWARD_WM_MDIACTIVATE(hwnd, fActive, hwndActivate, hwndDeactivate, DefMDIChildProc);
  return;
}

//
// BOOL DomWindowCreate(HWND hwndMdi, LPCREATESTRUCT lpCs)
//
// manages WM_CREATE on the Dom Window
//
// At this time, wParam and lParam not used
//
static BOOL NEAR DomWindowCreate(HWND hwndMdi, LPCREATESTRUCT lpCs)
{
  LPDOMDATA         lpDomData, lpRefDomData;
  HWND              hwndTreeLb;
  RECT              rc;
  LPMDICREATESTRUCT lpMcs;
  LPDOMNODE         lpDomNode;
  BOOL              bRet;
  LPDOMCREATEDATA   lpDomCreateData;  // dom creation temporary struct ptr

  // for caption update
  char              sz[BUFSIZE];
  UCHAR             VirtNodeName[MAXOBJECTNAME];

  // Get create parameters struct pointer received through lParam
  lpMcs     = (LPMDICREATESTRUCT) lpCs->lpCreateParams;
  lpDomCreateData = (LPDOMCREATEDATA) lpMcs->lParam;

  lpDomNode = lpDomCreateData->lpsDomNode;

  // allocate empty hooked structure
  lpDomData = (LPDOMDATA) ESL_AllocMem(sizeof(DOMDATA));
  if (!lpDomData) {
    CloseNodeStruct(lpDomNode->nodeHandle, TRUE);
    SetWindowLong(hwndMdi, 0, 0L);
    return FALSE;
  }
  SetWindowLong(hwndMdi, 0, (LONG) lpDomData);

  // initialize doc type
  lpDomData->typeDoc = TYPEDOC_DOM;

  // initialize type for caption
  switch (lpDomCreateData->domCreateMode) {
    case DOM_CREATE_QUERYNODE:
      lpDomData->domType = DOM_TYPE_NORMAL;
      lpDomData->domNumber = seqDomNormal;
      break;
    case DOM_CREATE_TEAROUT:
      lpDomData->domType = DOM_TYPE_TEAROUT;
      lpDomData->domNumber = seqDomTearOut;
      break;
    case DOM_CREATE_SCRATCHPAD:
      lpDomData->domType = DOM_TYPE_SCRATCHPAD;
      lpDomData->domNumber = seqDomScratchpad;
      break;
    case DOM_CREATE_NEWWINDOW:
      // take rid and number according to cloned dom type
      lpRefDomData = (LPDOMDATA)GetWindowLong(lpDomCreateData->hwndMdiRef, 0);
      lpDomData->domType = lpRefDomData->domType;
      switch (lpRefDomData->domType) {
        case DOM_TYPE_NORMAL:
          lpDomData->domNumber = seqDomNormal;
          break;
        case DOM_TYPE_TEAROUT:
          lpDomData->domNumber = seqDomTearOut;
          break;
        case DOM_TYPE_SCRATCHPAD:
          lpDomData->domNumber = seqDomScratchpad;
          break;
        case DOM_TYPE_REPOS:
          lpDomData->domNumber = seqDomRepos;
          break;
      }
      break;
  }

  // initialize show flags according to current setting
  lpDomData->bShowNanBar = QueryShowNanBarSetting(TYPEDOC_DOM);
  lpDomData->bShowStatus = QueryShowStatusSetting(TYPEDOC_DOM);

  // Note we are in create phase
  lpDomData->bCreatePhase = TRUE;

  // initialize currentRecId
  InitializeCurrentRecId(lpDomData);

  // initialize filters and flag bwithsystem
  lpDomData->lpBaseOwner[0]   = '\0';   // no filter on database owner
  lpDomData->lpOtherOwner[0]  = '\0';   // no filter on other objects owner
  lpDomData->bwithsystem      = FALSE;  // no system data

  // initialize gateway type and ingres version
  lpDomData->ingresVer   = GetOIVers();
  lpDomData->bIsVectorWise = IsVW();

  // anchor DomNode struct pointer received in parameter
  lpDomData->psDomNode = lpDomNode;

  // create Treelb window
  GetClientRect(hwndMdi, &rc);
  hwndTreeLb = LRCreateWindow(hwndMdi,
    &rc,
    WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE |
    LMS_WITHICONS   | LMS_AUTOHSCROLL | LMS_ALLONITEMMESSAGES,
    DomPref.b3d);
  if (hwndTreeLb == NULL) {
    // Emb 21/2/95 : let WM_DESTROY do the job
    //CloseNodeStruct(lpDomNode->nodeHandle, TRUE);
    //ESL_FreeMem (lpDomData->psDomNode);
    //ESL_FreeMem (lpDomData);
    return FALSE;
  }
  else
  {
      // Subclass the tree window proc

      lpDomData->lpNewTreeProc = MakeProcInstance(TreeSubclassWndProc, hInst);
      lpDomData->lpOldTreeProc = SubclassWindow(hwndTreeLb, lpDomData->lpNewTreeProc);
  }
  
  // Set the font
  if (IsGDIObject(hFontBrowsers))
    SendMessage(hwndTreeLb, LM_SETFONT, (WPARAM)hFontBrowsers, 0L);

  lpDomData->hwndTreeLb = hwndTreeLb;

  // Create Status bar and initialize it's font
  lpDomData->hwndStatus = NULL;

  // Create the tree status bar window
  lpDomData->hwndTreeStatus = CreateTreeStatusBarWnd(hwndMdi);
  if (!lpDomData->hwndTreeStatus)
    return FALSE;

  // initialize flag "Which Status?"
  lpDomData->bTreeStatus = TRUE;    // Tree status on top

  // Fill the button bar
  bRet = DomFillNanBar(hwndMdi, lpDomData);
  if (!bRet) {
    // Emb 21/2/95 : let WM_DESTROY do the job
    //CloseNodeStruct(lpDomNode->nodeHandle, TRUE);
    //ESL_FreeMem (lpDomData->psDomNode);
    //ESL_FreeMem (lpDomData);
    return FALSE;
  }

  // Fill the DOM tree - must be done AFTER nanbar special controls create
  bRet = DomFillTree(lpDomData,
                     lpDomCreateData->domCreateMode,
                     lpDomCreateData->hwndMdiRef,
                     lpDomCreateData->idFile);
  if (!bRet) {
    // Emb 21/2/95 : let WM_DESTROY do the job
    //TreeDeleteAllRecords(lpDomData);
    //CloseNodeStruct(lpDomNode->nodeHandle, TRUE);
    //ESL_FreeMem (lpDomData->psDomNode);
    //ESL_FreeMem (lpDomData);
    return FALSE;
  }

  // for teared-out doc: Recalculate the caption with the solved type
  if (lpDomCreateData->domCreateMode == DOM_CREATE_TEAROUT) {
    lstrcpy(VirtNodeName, GetVirtNodeName(lpDomData->psDomNode->nodeHandle));
    // Don't increment the dom number counter
    MakeDomCaption(sz, VirtNodeName, DOM_CREATE_TEAROUT, hwndMdi, FALSE);

  }

  // Update opened windows list in nodes bar
  switch (lpDomCreateData->domCreateMode) {
    case DOM_CREATE_TEAROUT:
    case DOM_CREATE_SCRATCHPAD:
    case DOM_CREATE_NEWWINDOW:
      {
        // request for refresh list of opened windows
        char *nodeName = GetVirtNodeName(lpDomData->psDomNode->nodeHandle);
        if (MfcDelayedUpdatesAccepted())
          MfcDelayUpdateOnNode(nodeName);
      }
      break;
  }
  return TRUE;    // Successful!
}

//
// BOOL DomWindowCommand(HWND hwndMdi, LPDOMDATA lpDomData, int id, HWND hwndCtl, UINT codeNotify)
//
// manages WM_COMMAND on the Dom Window
//
// At this time, hwndMdi not used
//
static BOOL NEAR DomWindowCommand(HWND hwndMdi, LPDOMDATA lpDomData, int id, HWND hwndCtl, UINT codeNotify)
{
  DWORD dwCurSel;     // Current selection on the tree
  WORD  wMessage;     // for "View" menu commands
  int   state;        // "system objects" checkbox state
  DWORD recId;        // all-use record id

  // for selchange right pane notify after collapse all
  DWORD dwOldCurSel, dwNewCurSel;

  // dummy instructions to skip compiler warnings at level four
  hwndMdi;

  // First switch for "View" menu commands management,
  // except expandbranch and expandbranch
  switch (id) {
    case IDM_CLASSB_EXPANDONE:
      wMessage=LM_EXPAND;
      break;
    case IDM_CLASSB_COLLAPSEONE:
      wMessage=LM_COLLAPSE;
      break;
    case IDM_CLASSB_COLLAPSEBRANCH:
      wMessage=LM_COLLAPSEBRANCH;
      break;
    case IDM_CLASSB_COLLAPSEALL:
      wMessage=LM_COLLAPSEALL;
      break;
    default:
      wMessage = 0;
      break;
  }
  if (wMessage) {
    dwCurSel=(DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
    if (dwCurSel) {
      if (id==IDM_CLASSB_EXPANDONE)
        gMemErrFlag = MEMERR_NOMESSAGE;

      if (id == IDM_CLASSB_COLLAPSEALL) {
        dwOldCurSel = GetCurSel(lpDomData);
        MfcBlockUpdateRightPane();
      }

      SendMessage(lpDomData->hwndTreeLb, wMessage, 0, (LPARAM)dwCurSel);

      if (id == IDM_CLASSB_COLLAPSEALL) {
        MfcAuthorizeUpdateRightPane();
        dwNewCurSel = GetCurSel(lpDomData);
        if (dwNewCurSel != dwOldCurSel) {
          UpdateRightPane(hwndMdi, lpDomData, TRUE, 0);
          lpDomData->dwSelBeforeRClick = dwNewCurSel;
        }
      }

      if (id==IDM_CLASSB_EXPANDONE) {
        if (gMemErrFlag == MEMERR_HAPPENED)
          MessageBox(GetFocus(),
                    ResourceString(IDS_E_MEMERR_EXPANDONE),
                    NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        gMemErrFlag = MEMERR_STANDARD;
      }
    }
    return TRUE;    // Processed (even if no current selection!)
  }

  // Second switch for other commands
  switch (id) {

    case IDM_CLASSB_EXPANDBRANCH:
      // only if at least one item
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L)) {
        char  rsCp[BUFSIZE];
        char  rsBuf[BUFSIZE];
        recId = SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM, 0, 0L);
        gMemErrFlag = MEMERR_NOMESSAGE;

        // ask for user confirmation
        LoadString(hResource, IDS_EXPANDDIRECT, rsCp, sizeof(rsCp));
        LoadString(hResource, IDS_EXPAND_LONG, rsBuf, sizeof(rsBuf));
        if (MessageBox(hwndMdi, rsBuf, rsCp, MB_ICONQUESTION|MB_YESNO | MB_TASKMODAL) == IDYES) {
          int retval = ManageExpandBranch(hwndMdi, lpDomData, 0L, NULL);
          if (retval == RES_ERR)
            //"Expansion interrupted by user action"
            MessageBox(GetFocus(),ResourceString(IDS_ERR_EXPANSION_INTERRUPTED) ,
                       rsCp,
                       MB_OK | MB_ICONEXCLAMATION);
          if (gMemErrFlag == MEMERR_HAPPENED)
            MessageBox(GetFocus(),
                      ResourceString(IDS_E_MEMERR_EXPANDBRANCH),
                      NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        }
        gMemErrFlag = MEMERR_STANDARD;
        SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)recId);
      }
      break;

    case IDM_CLASSB_EXPANDALL:
      // only if at least one item
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L)) {
        char  rsCp[BUFSIZE];
        char  rsBuf[BUFSIZE];
        recId = SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM, 0, 0L);
        gMemErrFlag = MEMERR_NOMESSAGE;

        // ask for user confirmation
        LoadString(hResource, IDS_EXPANDALL, rsCp, sizeof(rsCp));
        LoadString(hResource, IDS_EXPAND_LONG, rsBuf, sizeof(rsBuf));
        if (MessageBox(hwndMdi, rsBuf, rsCp, MB_ICONQUESTION|MB_YESNO | MB_TASKMODAL) == IDYES) {
          int retval = ManageExpandAll(hwndMdi, lpDomData, NULL);
          if (retval == RES_ERR)
            MessageBox(GetFocus(),
                       ResourceString(IDS_ERR_EXPANSION_INTERRUPTED),
                       rsCp,
                       MB_OK | MB_ICONEXCLAMATION);
          if (gMemErrFlag == MEMERR_HAPPENED)
            MessageBox(GetFocus(),
                      ResourceString(IDS_E_MEMERR_EXPANDALL),
                      NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        }
        gMemErrFlag = MEMERR_STANDARD;
        SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)recId);
      }
      break;

    case IDM_SHOW_SYSTEM:
      // Toggle the "show system" flag
      lpDomData->bwithsystem = !(lpDomData->bwithsystem);

      // Update checkbox according to new state
      state = (int)SendMessage(lpDomData->hwndSystem, BM_GETCHECK, 0, 0L);
      SendMessage(lpDomData->hwndSystem, BM_SETCHECK, !((BOOL)(state)), 0L);

      // Debug Emb 26/6/95 : performance measurement data
      #ifdef DEBUGMALLOC
      // Put a breakpoint in lbtree.cpp on ResetDebugEmbCounts()
      SendMessage(lpDomData->hwndTreeLb, LM_RESET_DEBUG_EMBCOUNTS, 0,0L);
      #endif

      // Update tree according to new state
      gMemErrFlag = MEMERR_NOMESSAGE;
      DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
      DOMUpdateDisplayData (
              lpDomData->psDomNode->nodeHandle, // hnodestruct
              OT_VIRTNODE,                      // iobjecttype
              0,                                // level
              NULL,                             // parentstrings
              FALSE,                            // bWithChildren
              ACT_CHANGE_SYSTEMOBJECTS,         // systemobjects flag
              0L,                               // no item id
              GetVdbaDocumentHandle(hwndMdi));  // current mdi window
      DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                  ResourceString(IDS_E_MEMERR_SHOWSYSTEM),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      gMemErrFlag = MEMERR_STANDARD;

      // Debug Emb 26/6/95 : performance measurement data
      #ifdef DEBUGMALLOC
      // Put a breakpoint in lbtree.cpp on ResetDebugEmbCounts()
      SendMessage(lpDomData->hwndTreeLb, LM_RESET_DEBUG_EMBCOUNTS, 0,0L);
      #endif

      // update the checkmark
      UpdateShowSystemMenuItem(lpDomData);

      return TRUE;
      break;

    case IDM_FILTER:
      ManageFiltersBox(hwndMdi, lpDomData);
      return TRUE;
      break;

    // manage focus for special objects on icons bar when mouse click
    case IDM_DUMMY1:
      SetFocus(lpDomData->hwndBaseOwner);
      // simulate click on control
      PostMessage(lpDomData->hwndBaseOwner, WM_LBUTTONDOWN,
                  0, GetMessagePos());
      return TRUE;
      break;
    case IDM_DUMMY2:
      SetFocus(lpDomData->hwndOtherOwner);
      // simulate click on control
      PostMessage(lpDomData->hwndOtherOwner, WM_LBUTTONDOWN,
                  0, GetMessagePos());
      return TRUE;
      break;
    case IDM_DUMMY3:
      SetFocus(lpDomData->hwndSystem);
      // simulate click on control
      PostMessage(lpDomData->hwndSystem, WM_LBUTTONDOWN, 0, GetMessagePos());
      return TRUE;
      break;

    // Manage Add/Alter/Drop object
    case IDM_ADDOBJECT:
      DomAddObject(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;
    case IDM_ALTEROBJECT:
      DomAlterObject(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;
    case IDM_DROPOBJECT:
      if (DomDropObject(hwndMdi, lpDomData, FALSE, (LPARAM)NULL)) {
        DomUpdateMenu(hwndMdi, lpDomData, TRUE);
        return TRUE;
      }
      else
        return FALSE;
      break;

    // Manage Grant/Revoke menuitems
    case IDM_GRANT:
      DomGrantObject(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;
    case IDM_REVOKE:
      DomRevokeObject(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;

    // Star management
    case IDM_REGISTERASLINK:
      DomRegisterAsLink(hwndMdi, lpDomData, FALSE, NULL, NULL, (DWORD)NULL);
      break;

    // Star management
    case IDM_REFRESHLINK:
      DomRefreshLink(hwndMdi, lpDomData, FALSE, NULL, NULL, (DWORD)NULL);
      break;

    // Star management
    case IDM_REMOVEOBJECT:
      if (DomDropObject(hwndMdi, lpDomData, FALSE, 1L)) {   // 1L stands for "Remove"
        DomUpdateMenu(hwndMdi, lpDomData, TRUE);
        return TRUE;
      }
      else
        return FALSE;
      break;

    case IDM_NEWWINDOW:
      // Make a DOM type MDI child window as clone from the current doc
      // only if at least one item
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L))
        MakeNewDomMDIChild (DOM_CREATE_NEWWINDOW, hwndMdi, 0, 0);
      break;

    case IDM_TEAROUT:
      // Make a DOM type MDI child window as teared out from the current doc
      // only if at least one item
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L))
        MakeNewDomMDIChild (DOM_CREATE_TEAROUT, hwndMdi, 0, 0);
      break;

    case IDM_RESTARTFROMPOS:
      // only if at least one item in the current dom
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L))
        RestartFromPosition(hwndMdi, lpDomData);
      break;

    case IDM_PRINT:
      gMemErrFlag = MEMERR_NOMESSAGE;
      PrintDom(hwndMdi, lpDomData);
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                  ResourceString(IDS_E_MEMERR_PRINTDOM),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      gMemErrFlag = MEMERR_STANDARD;
      break;

    case IDM_REFRESH:
      // only if at least one item in the current dom
      if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L))
        RefreshTrees(hwndMdi, lpDomData);
      break;

    case IDM_REPLIC_INSTALL:
      InstallReplicatorTables(hwndMdi, lpDomData);
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);    // Emb Aug. 7, 95
      SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;

//JFS Begin
    case IDM_REPLIC_PROPAG:
      ReplicatorPropagate(hwndMdi, lpDomData);
      SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_BUILDSRV:
      ReplicatorBuildServer(hwndMdi, lpDomData);
      // Useless: SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_RECONCIL:
      ReplicatorReconcile(hwndMdi, lpDomData);
      // Useless: SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_DEREPLIC:
      SetDispErr(FALSE);
      ReplicatorDereplicate(hwndMdi, lpDomData);
      SetDispErr(TRUE);
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);    // Emb Aug. 7, 95
      SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_MOBILE:
      ReplicatorMobile(hwndMdi, lpDomData);
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);    // Emb Aug. 7, 95
      // Useless: SetReplicatorRequestMandatory(hwndMdi, lpDomData);
      break;
//JFS End

    case IDM_REPLIC_ARCCLEAN:
      ReplicatorArcclean(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_REPMOD:
      ReplicatorRepmod(hwndMdi, lpDomData);
      break;

    case IDM_REPLIC_CREATEKEYS:
      ReplicatorCreateKeys(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_ACTIVATE:
      ReplicatorActivate(hwndMdi, lpDomData);
      break;
    case IDM_REPLIC_DEACTIVATE:
      ReplicatorDeactivate(hwndMdi, lpDomData);
      break;

    case IDM_PROPERTIES:
	  myerror(ERR_OLDCODE); // properties now in right paned

      break;

    case IDM_SYSMOD:
      return (SysmodOperation(hwndMdi, lpDomData));
      break;

    case IDM_ALTERDB:
      return (AlterDB(hwndMdi, lpDomData));
      break;

    case IDM_POPULATE:
      return (Populate(hwndMdi, lpDomData));
      break;

    case IDM_EXPORT:
      return (Export(hwndMdi, lpDomData));
      break;

    case IDM_AUDIT:
      return (Audit(hwndMdi, lpDomData));
      break;

    case IDM_GENSTAT:
      return (GenStat(hwndMdi, lpDomData));
      break;

    case IDM_UNLOADDB:
      return (UnloadDBOperation(hwndMdi, lpDomData));
      break;

    case IDM_COPYDB:
      return (CopyDBOperation(hwndMdi, lpDomData));
      break;

    case IDM_CUT:
      ShowHourGlass();
      ClearDomObjectFromClipboard(hwndMdi);
      CopyDomObjectToClipboard(hwndMdi, lpDomData, FALSE);

      DomDropObject(hwndMdi, lpDomData, TRUE, 0L);  // Don't ask for confirm
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);
      RemoveHourGlass();
      return TRUE;
      break;

    case IDM_COPY:
      ShowHourGlass();
      ClearDomObjectFromClipboard(hwndMdi);
      CopyDomObjectToClipboard(hwndMdi, lpDomData, FALSE);
      DomUpdateMenu(hwndMdi, lpDomData, TRUE);
      RemoveHourGlass();
      break;

    case IDM_PASTE:
      ShowHourGlass();
      PasteDomObjectFromClipboard(hwndMdi, lpDomData, FALSE);
      RemoveHourGlass();
      break;

    case IDM_VERIFYDB:
      return (VerifyDB (hwndMdi, lpDomData));
      break;

    case IDM_CHECKPOINT:
      return (Checkpoint (hwndMdi, lpDomData));
      break;

    case IDM_ROLLFORWARD:
      return (RollForward (hwndMdi, lpDomData));
      break;

    case IDM_DISPSTAT:
      return (DisplayStat (hwndMdi, lpDomData));
      break;

    case IDM_LOCATE:
      ManageLocate(hwndMdi, lpDomData);
      break;

    case IDM_MODIFYSTRUCT:
        DomModifyObjectStruct(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;

    case IDM_CREATEIDX:
      DomCreateIndex(hwndMdi, lpDomData, (WPARAM)NULL, (LPARAM)NULL);
      break;

    // CPP Managed
    case IDM_FASTLOAD:
      return (MfcFastload(hwndMdi));
      break;
    case IDM_INFODB:
      return (MfcInfodb(hwndMdi));
      break;
    case IDM_DUPLICATEDB:
      return (MfcDuplicateDb(hwndMdi));
      break;
    case IDM_SECURITYAUDIT:
      return (MfcSecurityAudit(hwndMdi));
      break;
    case IDM_EXPIREDATE:
      return (MfcExpireDate(hwndMdi));
      break;
    case IDM_COMMENT:
      return (MfcComment(hwndMdi));
      break;
    case IDM_USERMOD:
      return (MfcUsermod(hwndMdi));
      break;
    case IDM_JOURNALING:
      return MfcJournaling(hwndMdi);
      break;

    default:
      if (!ManageNotifMsg(hwndMdi, lpDomData, id, hwndCtl, codeNotify))
        return FALSE;   // flag "not processed" in case called from main wnd
      break;
  }
  return TRUE;
}


//
// VOID DomWindowFind :
//
// manages the search on the Dom Window
//
//  27/2/95 : added a wraparound feature
//
static VOID NEAR DomWindowFind(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
  BOOL      bMatchCase;
  BOOL      bWholeWord;
  char FAR *pszFind;
  HWND      hwndTreeLb;
  DWORD     dwCurSel;
  DWORD     dwNewSel;
  DWORD     dwNextToCurSel;
  char      rsBuf[BUFSIZE];
  char      rsTitle[BUFSIZE];
  char      bufFind[BUFSIZE];
  LPSTR     pszRecString;
  char      bufRecString[BUFSIZE];
  BOOL      bFound;
  int       state;      // is the record expanded?
  LPSTR     pCur, pPrec;

  // list of acceptable separators when searching for whole word in a dom
  static char szWordSeparator[] = " .*[]()\t";

  bMatchCase = (wParam & FIND_MATCHCASE ? TRUE : FALSE);
  bWholeWord = (wParam & FIND_WHOLEWORD ? TRUE : FALSE);
  pszFind = (char FAR *)lParam;
  hwndTreeLb = lpDomData->hwndTreeLb;

  // copy searched string in local buffers, and prepare for insensitive search
  fstrncpy(bufFind, pszFind, sizeof(bufFind));
  if (!bMatchCase)
    AnsiUpper(bufFind);

  // loop starting from the current selection
  dwCurSel = (DWORD)SendMessage(hwndTreeLb, LM_GETSEL, 0, 0L);
  dwNewSel = SendMessage(hwndTreeLb, LM_GETNEXT, 0, (LPARAM)dwCurSel);
  dwNextToCurSel = dwNewSel;
  while (dwNewSel) {
    pszRecString = (LPSTR)SendMessage(hwndTreeLb, LM_GETSTRING,
                                                  0, (LPARAM)dwNewSel);
    if (pszRecString) {
      // copy string in local buffer, and prepare for insensitive search
      lstrcpy(bufRecString, pszRecString);
      if (!bMatchCase)
        AnsiUpper(bufRecString);

      // search
      if (bWholeWord) {
        bFound = FALSE;
        pPrec = bufRecString;
        while (1) {
          // search substring
          pCur = x_strstr(pPrec, bufFind);
          if (pCur == 0) {
            bFound = FALSE;
            break;    // Not found. Cannot be found anymore. Bueno
          }

          // check previous character is a separator or first in the string
          if (pCur > bufRecString)
            if (x_strchr(szWordSeparator, *(pCur-1) ) == 0) {
              pPrec = pCur + 1;
              continue;   // try next occurence in the string
            }

          // check ending character is a separator or terminates the string
          if (*(pCur+x_strlen(bufFind)) != '\0')
            if (x_strchr(szWordSeparator, *(pCur+x_strlen(bufFind)) ) == 0) {
              pPrec = pCur + 1;
              continue;   // try next occurence in the string
            }
          
          // all tests fit
          bFound = TRUE;
          break;
        }
      }
      else
        bFound = ((x_strstr(bufRecString, bufFind)==NULL) ? FALSE : TRUE );

      // use search result
      if (bFound) {
        // special management if no other than current
        if (dwNewSel == dwCurSel)
          break;

        // get item visibility state
        state = (int)SendMessage(hwndTreeLb, LM_GETVISSTATE,
                                             0, (LPARAM)dwNewSel);
        if (state!=FALSE && state!=-1) {
          // make item visible to the user
          SendMessage(hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)dwNewSel);
          // set selection on it
          SendMessage(hwndTreeLb, LM_SETSEL, 0, (LPARAM)dwNewSel);
          return;
        }
      }
    }

    // Next item, and exit condition for the while() loop
    // Wraparoud managed here!
    dwNewSel = SendMessage(hwndTreeLb, LM_GETNEXT, 0, (LPARAM)dwNewSel);
    if (dwNewSel==0)
      dwNewSel = SendMessage(hwndTreeLb, LM_GETFIRST, 0, (LPARAM)dwNewSel);
    if (dwNewSel == dwNextToCurSel)
      break;      // wrapped around the whole tree : not found
  }

  // not found : display message box - truncate text if too long
  rsBuf[0] = '\0';
  if (dwNewSel == dwCurSel)
    x_strcpy(rsBuf, "No Other Occurrences of %s");
  else
    LoadString(hResource, IDS_FIND_TREE_NOTFOUND, rsBuf, sizeof(rsBuf));
  fstrncpy(bufFind, pszFind, sizeof(bufFind));
   if (x_strlen(bufFind) > 30) {
	fstrncpy ((LPUCHAR)bufFind, (LPUCHAR)pszFind,30);
	x_strcat(bufFind,"...");
 
  }
  wsprintf(bufRecString, rsBuf, bufFind);
  rsTitle[0] = '\0';
  VDBA_GetBaseProductCaption(rsTitle,sizeof(rsTitle));
  EnableWindow(hDlgSearch, FALSE);  // because MB_TASKMODAL not enough!
  MessageBox(hwndMdi, bufRecString, rsTitle,
             MB_OK | MB_TASKMODAL | MB_ICONASTERISK);
  EnableWindow(hDlgSearch, TRUE);
  SetFocus(hDlgSearch);
}

//
// VOID DomWindowSize(HWND hwndMdi, LPDOMDATA lpDomData, UINT state, int cx, int cy)
//
// manages WM_SIZE on the Dom Window
//
static VOID NEAR DomWindowSize(HWND hwndMdi, LPDOMDATA lpDomData, UINT state, int cx, int cy)
{
  int   nanbarH;      // height of button bar
  int   statusH;      // height of status bar
  int   tStatusH;     // height of tree status bar
  WORD  clientH;      // height of the client area
  WORD  clientW;      // width of the client area
  BOOL  bShowNanBar;  // Is nanbar visible ?
  BOOL  bShowStatus;  // Is status bar visible ?

  // IMPORTANT NOTE : tree status height assumed same as nanbar height

  // get show flags out of the structure
  bShowNanBar = lpDomData->bShowNanBar;
  bShowStatus = lpDomData->bShowStatus;

  if (state != SIZE_MINIMIZED) {
    clientW = cx;
    clientH = cy;

    // get heights of nanbar, taking special controls height in account
    if (bShowNanBar)
      nanbarH = GetControlsMaxHeight(lpDomData);
    else
      nanbarH = 0;
    // force to 0 in mfc architecture
    nanbarH = 0;

    // get status bar height
    if (bShowStatus)
      statusH = (int)Status_GetHeight(lpDomData->hwndStatus);
    else
      statusH = 0;

    // get tree status bar height
    if (bShowStatus)
      tStatusH = GetTreeStatusBarHeight(lpDomData->hwndTreeStatus);
    else
      tStatusH = 0;

    // Merge to common height
    statusH = max(statusH, tStatusH);

    //
    // move windows appropriately :
    //

    // Tree window before nanbar
    MoveWindow(lpDomData->hwndTreeLb, 0, nanbarH, clientW, clientH-nanbarH-statusH, FALSE);

    // Status bar / Tree status bar
    SizeTreeStatusBarWnd(hwndMdi);  // will initialize fields sizes
    if (bShowStatus) {
      // Move both
      MoveWindow(lpDomData->hwndTreeStatus, 0, clientH-statusH, clientW, statusH, TRUE);
      MoveWindow(lpDomData->hwndStatus, 0, clientH-statusH, clientW, statusH, TRUE);

      // Make both visible
      if (!IsWindowVisible(lpDomData->hwndTreeStatus))
        ShowWindow(lpDomData->hwndTreeStatus, SW_SHOW);
      if (!IsWindowVisible(lpDomData->hwndStatus))
        ShowWindow(lpDomData->hwndStatus, SW_SHOW);

      // bring the right one to top
      if (lpDomData->bTreeStatus)
        BringWindowToTop(lpDomData->hwndTreeStatus);
      else
        BringWindowToTop(lpDomData->hwndStatus);
    }
    else {
      ShowWindow(lpDomData->hwndStatus, SW_HIDE);
      ShowWindow(lpDomData->hwndTreeStatus, SW_HIDE);
    }
  }
}

//
// BOOL DomFillNanBar(HWND hwndMdi, LPDOMDATA lpDomData)
//
//  Fills the nan bar with buttons for a DOM document
//
static BOOL NEAR DomFillNanBar(HWND hwndMdi, LPDOMDATA lpDomData)
{
  // Fully revized as of March 6, 98:
  // 1) get rid of old code for pure C toolbar
  // 2) manage comboboxes inexistence for datacomm/idms

  BOOL bHasBaseOwner     = FALSE;
  BOOL bHasOtherOwner    = FALSE;
  BOOL bHasSystemObjects = FALSE;

  // Base owner only for pure open-ingres
  bHasBaseOwner = FALSE;   // default
  if (lpDomData->ingresVer != OIVERS_NOTOI)
    bHasBaseOwner = TRUE;

  // Other owner not if idms/datacomm,
  bHasOtherOwner = TRUE;
  if (lpDomData->ingresVer == OIVERS_NOTOI)
    bHasOtherOwner = FALSE;

  // system objects : always
  bHasSystemObjects = TRUE;

  // combo box with list of owners, for database filter
  if (bHasBaseOwner) {
    lpDomData->hwndBaseOwner = GetBaseOwnerComboHandle(hwndMdi);
    SubclassCombo(lpDomData->hwndBaseOwner, lpDomData);
    if (lpDomData->hwndBaseOwner == NULL)
      return FALSE;
    if (!FillCombo(lpDomData, lpDomData->hwndBaseOwner, FALSE))
      return FALSE;
    SetComboSelection(lpDomData->hwndBaseOwner, lpDomData->lpBaseOwner);
  }

  // combo box with list of owners, for other objects filter
  if (bHasOtherOwner) {
    lpDomData->hwndOtherOwner = GetOtherOwnerComboHandle(hwndMdi);
    SubclassCombo(lpDomData->hwndOtherOwner, lpDomData);
    if (lpDomData->hwndOtherOwner == NULL)
      return FALSE;
    if (!FillCombo(lpDomData, lpDomData->hwndOtherOwner, FALSE))
      return FALSE;
    SetComboSelection(lpDomData->hwndOtherOwner, lpDomData->lpOtherOwner);
  }

  // check box for system objects
  if (bHasSystemObjects) {
    lpDomData->hwndSystem = GetSystemCheckboxHandle(hwndMdi);
    SubclassCheckbox(lpDomData->hwndSystem, lpDomData);
    if (lpDomData->hwndSystem == NULL)
      return FALSE;
    SetCheckboxState(lpDomData->hwndSystem, lpDomData->bwithsystem);
  }

  return TRUE;
}

//
//  Set the size and position of the special controls on the tool bar
//
//  To be called after controls creation, or if the font used to draw
//  the tool bar has been changed.
//


//
//  returns the maximum height of the special controls on the tool bar
//
//
static int NEAR GetControlsMaxHeight(LPDOMDATA lpDomData)
{
  int   nanbarH;
  int   comboH    = 0;
  // get nanbar height as a minimum
  nanbarH = NanBar_GetHeight(lpDomData->hwndNanBar);


 // return the highest
 return (comboH+4 > nanbarH ? comboH+4: nanbarH);
}

//
// BOOL DomFillTree
//
//  Fills the lbtree with anchor lines
//
static BOOL NEAR DomFillTree(LPDOMDATA lpDomData, int domCreateMode, HWND hwndMdiRef, FILEIDENT idFile)
{
  DWORD         recIdSt;
  DWORD         recIdSub;
  UCHAR         buf[MAXOBJECTNAME];
  LPTREERECORD  lpRecord;
  LPDOMDATA     lpRefDomData;

  // for new window / tear out
  DWORD         tearedRecId;
  DWORD         recId;
  DWORD         lastCollapsedRecId;
  DWORD         refParentRecId;
  DWORD         stopLevel;
  DWORD         curLevel;
  LPTREERECORD  lpRefData, lpData;
  LINK_RECORD   lr;
  LPSTR         lpString;

  // for Loaded environment window
  int             retval;
  DOMDATAINFO     loadDomDataInfo;
  TREERECORD      sTreeRec;
  COMPLIMTREE     sComplimTree;
  COMPLIMTREELINE sComplimLine;
  long            cpt;
  int             previousExpansionState;
  DWORD           previousItemId;

  // for on the fly solve special management
  int   oldtype, newtype;

  // Special for right pane management
  DWORD curSelRecId = 0xFFFFFFFF;

  switch (domCreateMode) {
    case DOM_CREATE_LOAD:

      // Step 2.2: read the domdata partial structure with stored data
      retval = DBAReadData(idFile, (void *)(LPDOMDATAINFO)&loadDomDataInfo,
                                   sizeof(DOMDATAINFO));
      if (retval != RES_SUCCESS)
        return FALSE;

      // Update corresponding fields in the current domdata
      // Note : OpenNodeStruct must NOT be called, the stored node handle
      // is guaranteed valid.
      lpDomData->psDomNode->nodeHandle  = loadDomDataInfo.nodeHandle;
      lpDomData->currentRecId           = loadDomDataInfo.currentRecId;
      lpDomData->bTreeStatus            = loadDomDataInfo.bTreeStatus;
      lpDomData->bwithsystem            = loadDomDataInfo.bwithsystem;
      //lpDomData->gatewayType            = loadDomDataInfo.gatewayType;
      lstrcpy(lpDomData->lpBaseOwner,  loadDomDataInfo.lpBaseOwner);
      lstrcpy(lpDomData->lpOtherOwner, loadDomDataInfo.lpOtherOwner);
      lpDomData->domType                = loadDomDataInfo.domType;
      lpDomData->domNumber              = loadDomDataInfo.domNumber;

      // update caption type counters
      switch (loadDomDataInfo.domType) {
        case DOM_TYPE_NORMAL:
          if (seqDomNormal < loadDomDataInfo.domNumber)
            seqDomNormal = loadDomDataInfo.domNumber;
          break;
        case DOM_TYPE_TEAROUT:
          if (seqDomTearOut < loadDomDataInfo.domNumber)
            seqDomTearOut = loadDomDataInfo.domNumber;
          break;
        case DOM_TYPE_SCRATCHPAD:
          if (seqDomScratchpad < loadDomDataInfo.domNumber)
            seqDomScratchpad = loadDomDataInfo.domNumber;
          break;
        case DOM_TYPE_REPOS:
          if (seqDomRepos < loadDomDataInfo.domNumber)
            seqDomRepos = loadDomDataInfo.domNumber;
          break;
      }

      // update corresponding tool bar controls if necessary
      UpdateShowSystemMenuItem(lpDomData);    // Checkmark in menuitem
      if (lpDomData->bwithsystem)
        SendMessage(lpDomData->hwndSystem, BM_SETCHECK, 1, 0L);
        if (lpDomData->lpBaseOwner[0] != '\0') {
          FillCombo(lpDomData, lpDomData->hwndBaseOwner, TRUE); // Force fill
          SetComboSelection(lpDomData->hwndBaseOwner, lpDomData->lpBaseOwner);
        }
      if (lpDomData->lpOtherOwner[0] != '\0') {
        FillCombo(lpDomData, lpDomData->hwndOtherOwner, TRUE); // Force fill
        SetComboSelection(lpDomData->hwndOtherOwner, lpDomData->lpOtherOwner);
      }

      //
      // build tree by loading tree lines from the file
      //
      // Step 2.3: load number of items, selected and first visible line
      retval = DBAReadData(idFile, (void *)(LPCOMPLIMTREE)&sComplimTree,
                                   sizeof(COMPLIMTREE));
      if (retval != RES_SUCCESS)
        return FALSE;

      // Step 2.4: load the items of the tree
      lastCollapsedRecId = 0L;  // speed
      previousExpansionState = TRUE;  // no prev. item ---> like not expanded
      previousItemId         = 0L;    // no previous id
      for (cpt = 0; cpt < sComplimTree.count_all; cpt++) {

        // 2.4.1: load tree record
        retval = DBAReadData(idFile, (void *)(LPTREERECORD)&sTreeRec,
                                     sizeof(TREERECORD));
        if (retval != RES_SUCCESS)
          return FALSE;
        lpData = AllocAndFillRecord(0, FALSE, NULL, NULL, NULL, 0,
                                    NULL, NULL, NULL, NULL);    // dummy fill
        if (!lpData)
          return FALSE;   // otherwise gpf!
        _fmemcpy(lpData, (LPTREERECORD)&sTreeRec,
                         sizeof(TREERECORD));   // real fill

        // 2.4.2: load tree line caption, id, parent id, opening state
        retval = DBAReadData(idFile,
                             (void *)(LPCOMPLIMTREELINE)&sComplimLine,
                             sizeof(COMPLIMTREELINE));
        if (retval != RES_SUCCESS)
          return FALSE;

        // Special for right pane management
        // note : always reset whatever sComplimLine.recId is sComplimTree.curRecId or not,
        // since current line will be updated by OnInitDialog() of right pane
        lpData->bPageInfoCreated = FALSE;
        lpData->m_pPageInfo = 0;

        lr.lpString       = sComplimLine.caption;
        lr.ulRecID        = sComplimLine.recId;
        lr.ulParentID     = sComplimLine.parentRecId;
        lr.ulInsertAfter  = 0;    // we don't care since sequential inserts
        lr.lpItemData     = lpData;

        if (!SendMessage(lpDomData->hwndTreeLb,
                         LM_ADDRECORD,
                         0,
                         (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr)) {
          ESL_FreeMem(lpData);  // Must do since we don't call TreeAddRecord
          return FALSE;
        }

#ifdef WIN32
        // GENERIC BUG FIX 15/05/96 Emb :
        // update previous item state if necessary
        // for windows95 dll, we must force expansion
        // (we had to force collapse for non-windows95 lbtree)
        if (previousItemId)
          if (previousExpansionState)
            // regular expansion
            retval = SendMessage(lpDomData->hwndTreeLb, LM_EXPAND,
                                 0L, (LPARAM)previousItemId);

        // store current item collapsed/expanded state and id
        previousExpansionState = sComplimLine.recState;
        previousItemId         = sComplimLine.recId;

#else   // WIN32 :

#endif  // WIN32

        //loop on next item (for)
      }

      // full collapse of the last one - first expand since already collapsed
      if (lastCollapsedRecId) {
        SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0,
                                           (LPARAM)lastCollapsedRecId);

        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0,
                                           (LPARAM)lastCollapsedRecId);
      }

      // set current selection and first visible item,
      // except if empty scratchpad saved (makes a GPF in lbtree dll)
      if (sComplimTree.count_all > 0) {
        MfcBlockUpdateRightPane();

        // Set current selection
        SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0,
                                         sComplimTree.curRecId);
        // Set first visible item
        SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0,
                                        sComplimTree.FirstVis);

        MfcAuthorizeUpdateRightPane();
      }

      break;

    case DOM_CREATE_QUERYNODE:
      // build tree from scratch, except if context-driven with single item on root branch
      if (AutomatedDomDocWithRootItem()) {
        BOOL bSuccess = CreateAutomatedDomRootItem(lpDomData);
        if (bSuccess) {
          break;
        }
        // otherwise, create regular branches in sequence after having warned the end-user
        //"Cannot create tree item - creating whole tree"
        MessageBox(GetFocus(), ResourceString(IDS_ERR_TREE_ITEM), NULL, MB_OK | MB_ICONEXCLAMATION);
      }

      // Databases
      LoadString(hResource, IDS_TREE_DATABASE_STATIC, buf, sizeof(buf));
      lpRecord = AllocAndFillRecord(OT_STATIC_DATABASE, FALSE,
                                    NULL, NULL, NULL, 0,
                                    buf, NULL, NULL, NULL);
      if (!lpRecord)
        return FALSE;
      recIdSt = TreeAddRecord(lpDomData, buf, 0, 0, 0, lpRecord);
      if (recIdSt == 0)
        return FALSE;
      recIdSub = AddDummySubItem(lpDomData, recIdSt);
      if (recIdSub == 0)
        return FALSE;

      #ifdef DEBUG_TRACE_ALLOCS_AND_FREE
      {
        LPVOID lpItemData;
        lpItemData = (LPVOID) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA,
                                          0, (LPARAM)recIdSt);
        lpItemData = (LPVOID) SendMessage(lpDomData->hwndTreeLb, LM_GETITEMDATA,
                                          0, (LPARAM)recIdSub);
        return TRUE;
      }
      #endif

        if (GetOIVers() == OIVERS_NOTOI) {
          // Restricted features if gateway

          // No Users anymore, replaced by subbranch schemausers
          // under each database
        }
        else {
          // not a gateway : full features
          // Profiles
          LoadString(hResource, IDS_TREE_PROFILE_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_PROFILE, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;
        
          // Users
          LoadString(hResource, IDS_TREE_USER_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_USER, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;
        
          // Groups
          LoadString(hResource, IDS_TREE_GROUP_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_GROUP, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;
        
          // Roles
          LoadString(hResource, IDS_TREE_ROLE_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_ROLE, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;
        
          // Locations
          LoadString(hResource, IDS_TREE_LOCATION_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_LOCATION, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;

          // ICE
          /*LoadString(hResource, IDS_TREE_ICE_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_ICE, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
	  */
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;

          // INSTALLATION LEVEL SETTINGS
          LoadString(hResource, IDS_TREE_INSTALL_STATIC, buf, sizeof(buf));
          lpRecord = AllocAndFillRecord(OT_STATIC_INSTALL, FALSE,
                                        NULL, NULL, NULL, 0,
                                        buf, NULL, NULL, NULL);
          if (!lpRecord)
            return FALSE;
          recIdSt = TreeAddRecord(lpDomData, buf, 0, recIdSt, 0, lpRecord);
          if (recIdSt == 0)
            return FALSE;
          recIdSub = AddDummySubItem(lpDomData, recIdSt);
          if (recIdSub == 0)
            return FALSE;
        }

      // Show DOM tree in collapsed mode
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSEALL, 0, 0L);

      break;

    case DOM_CREATE_NEWWINDOW:
    case DOM_CREATE_TEAROUT:
    case DOM_CREATE_SCRATCHPAD:

      // for scratchpad : only if referred mdi doc is a dom
      if (domCreateMode == DOM_CREATE_SCRATCHPAD) {
        if (!hwndMdiRef)
          break;
        if (QueryDocType(hwndMdiRef) != TYPEDOC_DOM)
          break;
      }

      if (hwndMdiRef)
        lpRefDomData = (LPDOMDATA)GetWindowLong(hwndMdiRef, 0);
      else
        lpRefDomData = 0;

      // Special for right pane management
      if (domCreateMode == DOM_CREATE_NEWWINDOW || domCreateMode == DOM_CREATE_TEAROUT)
        curSelRecId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
      else
        curSelRecId = 0xFFFFFFFF; // will never be found

      // only for NEWWINDOW and DOM_CREATE_TEAROUT  :
      // Update filters and bwithsystem flags
      if (domCreateMode == DOM_CREATE_NEWWINDOW
          || domCreateMode == DOM_CREATE_TEAROUT) {

        lpDomData->bwithsystem = lpRefDomData->bwithsystem;
        lstrcpy(lpDomData->lpBaseOwner,  lpRefDomData->lpBaseOwner);
        lstrcpy(lpDomData->lpOtherOwner, lpRefDomData->lpOtherOwner);

        // update corresponding tool bar controls if necessary
        if (lpDomData->bwithsystem)
          SendMessage(lpDomData->hwndSystem, BM_SETCHECK, 1, 0L);
          if (lpDomData->lpBaseOwner[0] != '\0') {
            FillCombo(lpDomData, lpDomData->hwndBaseOwner, TRUE); // Force fill
            SetComboSelection(lpDomData->hwndBaseOwner, lpDomData->lpBaseOwner);
          }
        if (lpDomData->lpOtherOwner[0] != '\0') {
          FillCombo(lpDomData, lpDomData->hwndOtherOwner, TRUE); // Force fill
          SetComboSelection(lpDomData->hwndOtherOwner, lpDomData->lpOtherOwner);
        }
      }

      // Empty tree for scratchpad
      if (domCreateMode == DOM_CREATE_SCRATCHPAD)
        break;

      // Fill the tree
      if (domCreateMode == DOM_CREATE_NEWWINDOW) {
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
        stopLevel = (DWORD)(-1L);                 // will never be reached!
        tearedRecId = lpRefDomData->currentRecId; // will never be reached!
      }
      else {
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
        stopLevel = SendMessage(lpRefDomData->hwndTreeLb, LM_GETLEVEL,
                                0, (LPARAM)recId);
        tearedRecId = recId;    // for parenthood special management
      }
      while (recId) {
        // get information about the line in the referred tree
        lpRefData = (LPTREERECORD)SendMessage(lpRefDomData->hwndTreeLb,
                                              LM_GETITEMDATA,
                                              0, (LPARAM)recId);
        if (lpRefData) {
          lpData = AllocAndFillRecord(0, FALSE, NULL, NULL, NULL, 0,
                                      NULL, NULL, NULL, NULL);  // dummy fill
          if (lpData)
            _fmemcpy(lpData, lpRefData, sizeof(TREERECORD));  // real fill
          else
            return FALSE;

          // Special management to clear right pane references
          if (lpData->bPageInfoCreated)
            lpData->bPageInfoCreated = FALSE;
          if (lpData->m_pPageInfo)
            lpData->m_pPageInfo = 0;
        }
        else
          lpData = NULL;    // no extra data

        lpString = (LPSTR)SendMessage(lpRefDomData->hwndTreeLb,
                                      LM_GETSTRING, 0, (LPARAM)recId);
        if (recId!=tearedRecId)
          refParentRecId = SendMessage(lpRefDomData->hwndTreeLb,
                                      LM_GETPARENT, 0, (LPARAM)recId);
        else {
          // No parent for the teared out record
          refParentRecId = 0L;
          // Change on the fly for a basic type, with on-the-fly type solve
          oldtype = lpData->recType;
          newtype = SolveType(lpDomData, lpData, TRUE);
          // Note that lpData->recType, parenthood, etc have been updated
          // by calling SolveType since the last parameter was TRUE
          lpData->recType = GetBasicType(lpData->recType);
        }

        // create equivalent line in the new tree
        // hand create preferred to TreeAddRecord not to update currentRecId
        lr.lpString       = lpString;
        lr.ulRecID        = recId;
        lr.ulParentID     = refParentRecId;
        lr.ulInsertAfter  = 0;    // we don't care since sequential inserts
        lr.lpItemData     = lpData;

        if (!SendMessage(lpDomData->hwndTreeLb,
                         LM_ADDRECORD,
                         0,
                         (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr)) {
          ESL_FreeMem(lpData);  // Must do since we don't call TreeAddRecord
          return FALSE;
        }

        // get next
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETNEXT,
                            0, (LPARAM)recId);
        curLevel = SendMessage(lpRefDomData->hwndTreeLb, LM_GETLEVEL,
                               0, (LPARAM)recId);

        // exit loop decision - cast as signed to take the -1 in account!
        if ((LONG)curLevel <= (LONG)stopLevel)
          break;
      }

      // Second loop to set the expanded/collapsed mode for each item
      // we loop on the new dom, assuming the record id's are strictly
      // identical on both sides.
      // updated 25/9/95 Emb for speed
      recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
      lastCollapsedRecId = 0L;
      while (recId) {
        if (SendMessage(lpRefDomData->hwndTreeLb,
                        LM_GETSTATE, 0, (LPARAM)recId) == FALSE) {
          // fast collapse
          SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0xdcba,
                                             (LPARAM)recId);
          lastCollapsedRecId = recId;
        }
#ifdef WIN32
        else {
          // GENERIC BUG FIX 15/05/96 Emb :
          // else case for win32 dll : need to explicitly expand
          SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0, (LPARAM)recId);
        }
#endif  // WIN32
        recId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                            0, (LPARAM)recId);
      }
      // full collapse of the last one - first expand since already collapsed
      if (lastCollapsedRecId) {
        SendMessage(lpDomData->hwndTreeLb, LM_EXPAND, 0,
                                           (LPARAM)lastCollapsedRecId);

        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0,
                                           (LPARAM)lastCollapsedRecId);
      }

      // update start value for current record id
      lpDomData->currentRecId = lpRefDomData->currentRecId;

      // specific code for teared out doms:
      if (domCreateMode == DOM_CREATE_TEAROUT) {
        // set the parent sub-branch if necessary
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
        stopLevel = SendMessage(lpRefDomData->hwndTreeLb, LM_GETLEVEL,
                                0, (LPARAM)recId);
        if (stopLevel > 0) {
          recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
          if (!AddParentBranch(lpDomData, recId))
            return FALSE;
        }
      }

      // specific code for newwindow doms
      // and teared out windows:
      // Set current selection, but delay right pane update
      if (domCreateMode == DOM_CREATE_NEWWINDOW) {
        // set the selection and top item as the parent
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
        MfcBlockUpdateRightPane();
        SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)recId);
        MfcAuthorizeUpdateRightPane();
        recId = SendMessage(lpRefDomData->hwndTreeLb, LM_GETTOPITEM, 0, 0L);
        SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0, (LPARAM)recId);
      }
      else if (domCreateMode == DOM_CREATE_TEAROUT) {
        // set the first item as current selection
        recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
        MfcBlockUpdateRightPane();
        SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)recId);
        MfcAuthorizeUpdateRightPane();
      }
      
      // Note : no need to change the checkmark state on the "Show system
      // objects" menuitem, since it already has the "good" value
      // UpdateShowSystemMenuItem(lpDomData);

      // That's all folks!
      break;
  }

  return TRUE;
}

//
//  Adds a branch with the parent, if necessary
//
//  Calls itself in case the parent branch must have a parent branch
//  (example : integrities -> parent table -> parent database
//
BOOL AddParentBranch(LPDOMDATA lpDomData, DWORD recId)
{
  DWORD         idChildObj;
  LPTREERECORD  lpRecord, lpRecordChild;
  char          buf[BUFSIZE];
  char          bufRsParent[BUFSIZE];
  char          bufRsObjType[BUFSIZE];
  BOOL          recState;
  int           iret;
  int           resultType;
  int           resultLevel;
  LPUCHAR       aparentsResult[MAXPLEVEL];
  LPUCHAR       aparents[MAXPLEVEL];
  BOOL          bNoDummy;

  // check recId Validity
  if (recId == 0)
    return FALSE;

  // get it's data
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                        LM_GETITEMDATA,
                                        0, (LPARAM)recId);
  if (!lpRecord)
    return FALSE;

  // Do we already have a sub-item?
  if (GetFirstImmediateChild(lpDomData, recId, 0))
    bNoDummy = FALSE;
  else
    bNoDummy = TRUE;

  // if no dummy sub-item, set sub-branch state to TRUE
  if (bNoDummy)
    lpRecord->bSubValid = TRUE;

  // get record state - simulate collapsed if no dummy sub-item
  recState = (BOOL)SendMessage(lpDomData->hwndTreeLb ,
                               LM_GETSTATE, 0, (LPARAM)recId);
  if (bNoDummy)
    recState = 0;

  // check parent branch not already there!
  if (lpRecord->parentbranch & PARENTBRANCH_THERE)
    return TRUE;

  // if not expanded, take note that the branch will have to be created
  // at expand time
  if (lpRecord->bSubValid == FALSE) {
    lpRecord->parentbranch |= PARENTBRANCH_NEEDED;
    return TRUE;
  }

  switch (lpRecord->recType) {
    // straightforward types
    case OT_STATIC_TABLE:
    case OT_STATIC_VIEW:
    case OT_STATIC_PROCEDURE:
    case OT_STATIC_SEQUENCE:
    case OT_STATIC_SCHEMA:
    case OT_STATIC_SYNONYM:
    case OT_STATIC_DBEVENT:
    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
    case OT_SEQUENCE:
    case OT_SCHEMAUSER:
    case OT_SYNONYM:
    case OT_DBEVENT:
    case OT_STATIC_DBALARM:                 // alarms on database
    case OT_STATIC_DBALARM_CONN_SUCCESS:
    case OT_STATIC_DBALARM_CONN_FAILURE:
    case OT_STATIC_DBALARM_DISCONN_SUCCESS:
    case OT_STATIC_DBALARM_DISCONN_FAILURE:
    case OT_S_ALARM_CO_SUCCESS_USER:
    case OT_S_ALARM_CO_FAILURE_USER:
    case OT_S_ALARM_DI_SUCCESS_USER:
    case OT_S_ALARM_DI_FAILURE_USER:

    // for xrefs types : alarms
    case OTR_ALARM_SELSUCCESS_TABLE:
    case OTR_ALARM_SELFAILURE_TABLE:
    case OTR_ALARM_DELSUCCESS_TABLE:
    case OTR_ALARM_DELFAILURE_TABLE:
    case OTR_ALARM_INSSUCCESS_TABLE:
    case OTR_ALARM_INSFAILURE_TABLE:
    case OTR_ALARM_UPDSUCCESS_TABLE:
    case OTR_ALARM_UPDFAILURE_TABLE:
    // for xrefs types : tables on location
    case OTR_LOCATIONTABLE:
    // for xrefs types : grants
    case OTR_GRANTEE_SEL_TABLE:
    case OTR_GRANTEE_INS_TABLE:
    case OTR_GRANTEE_UPD_TABLE:
    case OTR_GRANTEE_DEL_TABLE:
    case OTR_GRANTEE_REF_TABLE:
    case OTR_GRANTEE_ALL_TABLE:
    case OTR_REPLIC_CDDS_TABLE:
    // Not necessary because same database as table : case OTR_TABLEVIEW:
    case OTR_GRANTEE_SEL_VIEW:
    case OTR_GRANTEE_INS_VIEW:
    case OTR_GRANTEE_UPD_VIEW:
    case OTR_GRANTEE_DEL_VIEW:
    case OTR_GRANTEE_EXEC_PROC:
    case OTR_GRANTEE_RAISE_DBEVENT:
    case OTR_GRANTEE_REGTR_DBEVENT:
    case OTR_GRANTEE_NEXT_SEQU:


      // add parent database branch
      lpRecordChild = AllocAndFillRecord(OT_DATABASE, FALSE,
                                         NULL,            // parent level 0
                                         NULL,            // parent level 1
                                         NULL,            // parent level 2
                                         lpRecord->parentDbType,
                                         lpRecord->extra, // base name
                                         NULL,            // owner
                                         NULL,            // complim
                                         NULL);           // table schema
      if (!lpRecordChild)
        return FALSE;       // Fatal

      // get owner name and complimentary data for the object to be added,
      // along with parenthood
      aparentsResult[0] = lpRecordChild->extra;
      aparentsResult[1] = lpRecordChild->extra2;
      aparentsResult[2] = lpRecordChild->extra3;
      lpRecordChild->ownerName[0] = '\0';   // Caution if no owner
      lpRecordChild->szComplim[0] = '\0';   // Caution if no owner
      iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                          lpRecord->extra,            // object name
                          "",                         // no owner name
                          OT_DATABASE,                // object type
                          0,                          // level
                          NULL,                       // aparents
                          lpDomData->bwithsystem,
                          &resultType,                // expect OT_DATABASE!
                          &resultLevel,
                          aparentsResult,
                          lpRecordChild->objName,     // result object name
                          lpRecordChild->ownerName,   // result owner
                          lpRecordChild->szComplim);  // result complim
      if (iret != RES_SUCCESS)
        return FALSE;

      LoadString(hResource, IDS_PARENT, bufRsParent, sizeof(bufRsParent));
      LoadString(hResource, GetObjectTypeStringId(OT_DATABASE),
                        bufRsObjType, sizeof(bufRsObjType));
      wsprintf(buf, "( %s %s ) %s", bufRsParent, bufRsObjType,
                                               lpRecordChild->objName);
      idChildObj = TreeAddRecord(lpDomData, buf, recId, 0, 0, lpRecordChild);
      if (idChildObj==0)
        return FALSE;

      // Keep note that the parent branch has been created.
      lpRecord->parentbranch |= PARENTBRANCH_THERE;
      lpRecord->parentbranch &= ~PARENTBRANCH_NEEDED;

      // Add a dummy sub-sub-item to this sub item
      if (AddDummySubItem(lpDomData, idChildObj)==0)
        return FALSE;

      // Set the sub item in collapsed mode
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)idChildObj);

      // Set the teared out item in collapsed mode if was collapsed
      if (recState == FALSE)
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                    0, (LPARAM)recId);
      break;

    case OT_STATIC_INTEGRITY:
    case OT_INTEGRITY:

    case OT_STATIC_RULE:
    case OT_RULE:

    case OT_STATIC_SECURITY:
    case OT_STATIC_SEC_SEL_SUCC:
    case OT_S_ALARM_SELSUCCESS_USER:
    case OT_STATIC_SEC_SEL_FAIL:
    case OT_S_ALARM_SELFAILURE_USER:
    case OT_STATIC_SEC_DEL_SUCC:
    case OT_S_ALARM_DELSUCCESS_USER:
    case OT_STATIC_SEC_DEL_FAIL:
    case OT_S_ALARM_DELFAILURE_USER:
    case OT_STATIC_SEC_INS_SUCC:
    case OT_S_ALARM_INSSUCCESS_USER:
    case OT_STATIC_SEC_INS_FAIL:
    case OT_S_ALARM_INSFAILURE_USER:
    case OT_STATIC_SEC_UPD_SUCC:
    case OT_S_ALARM_UPDSUCCESS_USER:
    case OT_STATIC_SEC_UPD_FAIL:
    case OT_S_ALARM_UPDFAILURE_USER:

    case OT_STATIC_INDEX:
    case OT_INDEX:

    case OT_STATIC_TABLELOCATION:
    case OT_TABLELOCATION:

    case OT_STATIC_R_TABLESYNONYM:
    case OTR_TABLESYNONYM:

    case OT_STATIC_R_TABLEVIEW:
    case OTR_TABLEVIEW:
      // 1) add parent table branch
      lpRecordChild = AllocAndFillRecord(OT_TABLE, FALSE,
                                         lpRecord->extra,   // parent level 0
                                         NULL,              // parent level 1
                                         NULL,              // parent level 2
                                         lpRecord->parentDbType,
                                         lpRecord->extra2,  // table name
                                         NULL,              // owner
                                         NULL,              // complim
                                         NULL);             // table schema
      if (!lpRecordChild)
        return FALSE;       // Fatal

      // get owner name and complimentary data for the object to be added,
      // along with parenthood
      aparentsResult[0] = lpRecordChild->extra;
      aparentsResult[1] = lpRecordChild->extra2;
      aparentsResult[2] = lpRecordChild->extra3;
      aparents[0] = lpRecord->extra;        // One parenthood level
      aparents[1] = aparents[2] = NULL;
      lpRecordChild->ownerName[0] = '\0';   // Caution if no owner
      lpRecordChild->szComplim[0] = '\0';   // Caution if no owner
      iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                          lpRecord->extra2,           // table name
                          lpRecord->tableOwner,
                          OT_TABLE,                   // object type
                          1,                          // level
                          aparents,                   // aparents
                          lpDomData->bwithsystem,
                          &resultType,                // expect OT_TABLE!
                          &resultLevel,
                          aparentsResult,
                          lpRecordChild->objName,     // result object name
                          lpRecordChild->ownerName,   // result owner
                          lpRecordChild->szComplim);  // result complim
      if (iret != RES_SUCCESS) {
        ESL_FreeMem(lpRecordChild);
        return FALSE;
      }

      LoadString(hResource, IDS_PARENT, bufRsParent, sizeof(bufRsParent));
      LoadString(hResource, GetObjectTypeStringId(OT_TABLE),
                        bufRsObjType, sizeof(bufRsObjType));
      wsprintf(buf, "( %s %s ) %s", bufRsParent, bufRsObjType,
                                               lpRecordChild->objName);
      idChildObj = TreeAddRecord(lpDomData, buf, recId, 0, 0, lpRecordChild);
      if (idChildObj==0)
        return FALSE;

      // Keep note that the parent branch has been created.
      lpRecord->parentbranch |= PARENTBRANCH_THERE;
      lpRecord->parentbranch &= ~PARENTBRANCH_NEEDED;

      // Add a dummy sub-sub-item to this sub item
      if (AddDummySubItem(lpDomData, idChildObj)==0)
        return FALSE;

      // Set the sub item in collapsed mode
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)idChildObj);

      // Set the teared out item in collapsed mode if was collapsed
      if (recState == FALSE)
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                    0, (LPARAM)recId);

      // 2) add a parent database branch to the freshly created table branch
      // done by recursive call
      return AddParentBranch(lpDomData, idChildObj);

      break;

    case OT_STATIC_VIEWTABLE:
    case OT_VIEWTABLE:

    case OT_STATIC_R_VIEWSYNONYM:
    case OTR_VIEWSYNONYM:

      // 1) add parent table branch
      lpRecordChild = AllocAndFillRecord(OT_VIEW, FALSE,
                                         lpRecord->extra,   // parent level 0
                                         NULL,              // parent level 1
                                         NULL,              // parent level 2
                                         lpRecord->parentDbType,
                                         lpRecord->extra2,  // table name
                                         NULL,              // owner
                                         NULL,              // complim
                                         NULL);             // table schema
      if (!lpRecordChild)
        return FALSE;       // Fatal

      // get owner name and complimentary data for the object to be added,
      // along with parenthood
      aparentsResult[0] = lpRecordChild->extra;
      aparentsResult[1] = lpRecordChild->extra2;
      aparentsResult[2] = lpRecordChild->extra3;
      aparents[0] = lpRecord->extra;        // One parenthood level
      aparents[1] = aparents[2] = NULL;
      lpRecordChild->ownerName[0] = '\0';   // Caution if no owner
      lpRecordChild->szComplim[0] = '\0';   // Caution if no owner
      iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                          lpRecord->extra2,           // view name
                          lpRecord->tableOwner,       // compatibility only
                          OT_VIEW,                    // object type
                          1,                          // level
                          aparents,                   // aparents
                          lpDomData->bwithsystem,
                          &resultType,                // expect OT_VIEW!
                          &resultLevel,
                          aparentsResult,
                          lpRecordChild->objName,     // result object name
                          lpRecordChild->ownerName,   // result owner
                          lpRecordChild->szComplim);  // result complim
      if (iret != RES_SUCCESS) {
        ESL_FreeMem(lpRecordChild);
        return FALSE;
      }

      LoadString(hResource, IDS_PARENT, bufRsParent, sizeof(bufRsParent));
      LoadString(hResource, GetObjectTypeStringId(OT_VIEW),
                        bufRsObjType, sizeof(bufRsObjType));
      wsprintf(buf, "( %s %s ) %s", bufRsParent, bufRsObjType,
                                               lpRecordChild->objName);
      idChildObj = TreeAddRecord(lpDomData, buf, recId, 0, 0, lpRecordChild);
      if (idChildObj==0)
        return FALSE;

      // Keep note that the parent branch has been created.
      lpRecord->parentbranch |= PARENTBRANCH_THERE;
      lpRecord->parentbranch &= ~PARENTBRANCH_NEEDED;

      // Add a dummy sub-sub-item to this sub item
      if (AddDummySubItem(lpDomData, idChildObj)==0)
        return FALSE;

      // Set the sub item in collapsed mode
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)idChildObj);

      // Set the teared out item in collapsed mode if was collapsed
      if (recState == FALSE)
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                    0, (LPARAM)recId);

      // 2) add a parent database branch to the freshly created table branch
      // done by recursive call
      return AddParentBranch(lpDomData, idChildObj);

      break;

  }

  return TRUE;
}

//
//  Manage the Restart from position menuitem
//
static BOOL NEAR RestartFromPosition(HWND hwndMdi, LPDOMDATA lpDomData)
{
  HWND  hwndNewTree;
  RECT  rc;           // rectangle for the new dom
  /*
  int   nanbarH;      // height of button bar
  int   statusH;      // height of status bar
  */

  DWORD         tearedRecId;
  DWORD         recId;
  DWORD         refParentRecId;
  DWORD         stopLevel;
  DWORD         curLevel;
  LPTREERECORD  lpRefData, lpData;
  LINK_RECORD   lr;
  LPSTR         lpString;

  // for caption update
  char          sz[BUFSIZE];
  UCHAR         VirtNodeName[MAXOBJECTNAME];

  // Get current rectangle for treelb - fill whole hwndMdi since it is the left view
  GetClientRect(hwndMdi, &rc);

  /* old code:
  // get mdi client rect
  GetClientRect(hwndMdi, &rc);

  // get status bar and toolbar heights
  nanbarH = GetControlsMaxHeight(lpDomData);
  statusH = (int)Status_GetHeight(lpDomData->hwndStatus);

  // adjust rectangle
  rc.top      = nanbarH;
  rc.bottom  -= statusH;
  */

  // create Treelb window
  hwndNewTree = LRCreateWindow(hwndMdi,
    &rc,
    WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE |
    LMS_WITHICONS   | LMS_AUTOHSCROLL | LMS_ALLONITEMMESSAGES,
    DomPref.b3d);
  if (hwndNewTree == NULL)
    return FALSE;

  // Set the font
  if (IsGDIObject(hFontBrowsers))
    SendMessage(hwndNewTree, LM_SETFONT, (WPARAM)hFontBrowsers, 0L);

  // take the "good" lines in the original tree
  // refer to function "DomFillTree", from which this code is extracted
  // (it was more complicated to alter the function)

  recId = SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
  stopLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                          0, (LPARAM)recId);
  tearedRecId = recId;    // for parenthood special management

  while (recId) {
    // get information about the line in the referred tree
    lpRefData = (LPTREERECORD)SendMessage(lpDomData->hwndTreeLb,
                                          LM_GETITEMDATA,
                                          0, (LPARAM)recId);
    if (lpRefData) {
      lpData = AllocAndFillRecord(0, FALSE, NULL, NULL, NULL, 0,
                                  NULL, NULL, NULL, NULL);  // dummy fill
      if (lpData)
        _fmemcpy(lpData, lpRefData, sizeof(TREERECORD));  // real fill
      else
        return FALSE;

      // special management to preserve right pane for remaining items
      if (lpData->bPageInfoCreated) {
        lpRefData->bPageInfoCreated = FALSE;
        lpRefData->m_pPageInfo = 0;
      }
    }
    else
      lpData = NULL;    // no extra data

    lpString = (LPSTR)SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETSTRING, 0, (LPARAM)recId);
    if (recId!=tearedRecId)
      refParentRecId = SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETPARENT, 0, (LPARAM)recId);
    else {
      refParentRecId = 0L;    // No parent for the teared out record
      SolveType(lpDomData, lpData, TRUE);   // solve type on the fly
      lpData->recType = GetBasicType(lpData->recType);
    }

    // create equivalent line in the new tree
    // hand create preferred to TreeAddRecord not to update currentRecId
    lr.lpString       = lpString;
    lr.ulRecID        = recId;
    lr.ulParentID     = refParentRecId;
    lr.ulInsertAfter  = 0;    // we don't care since sequential inserts
    lr.lpItemData     = lpData;

    if (!SendMessage(hwndNewTree,
                     LM_ADDRECORD,
                     0,
                     (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr)) {
      ESL_FreeMem(lpData);  // Must do since we don't call TreeAddRecord
      return FALSE;
    }

    // get next
    recId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)recId);
    curLevel = SendMessage(lpDomData->hwndTreeLb, LM_GETLEVEL,
                           0, (LPARAM)recId);

    // exit loop decision - cast as signed to take the -1 in account!
    if ((LONG)curLevel <= (LONG)stopLevel)
      break;
  }

  // Second loop to set the expanded/collapsed mode for each item
  // we loop on the new dom, assuming the record id's are strictly
  // identical on both sides.
  recId = SendMessage(hwndNewTree, LM_GETFIRST, 0, 0L);
  while (recId) {
    if (SendMessage(lpDomData->hwndTreeLb,
                    LM_GETSTATE, 0, (LPARAM)recId) == FALSE)
      SendMessage(hwndNewTree, LM_COLLAPSE, 0, (LPARAM)recId);
    recId = SendMessage(hwndNewTree, LM_GETNEXT,
                        0, (LPARAM)recId);
  }

  // delete the old tree
  // leave subclassing stuff as is
  TreeDeleteAllRecords(lpDomData);
#ifdef WIN32
  {
    OSVERSIONINFO monster;
    monster.dwOSVersionInfoSize = sizeof(monster);
    if (GetVersionEx(&monster)) {
      if (monster.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        ShowWindow(lpDomData->hwndTreeLb, SW_HIDE);
      else
        DestroyWindow(lpDomData->hwndTreeLb);
    }
    else
      DestroyWindow(lpDomData->hwndTreeLb);
  }
#else
  DestroyWindow(lpDomData->hwndTreeLb);
#endif

  // put the new tree instead
  lpDomData->hwndTreeLb = hwndNewTree;
  // leave subclassing stuff as is
  //lpDomData->lpNewTreeProc = MakeProcInstance(TreeSubclassWndProc, hInst);
  //lpDomData->lpOldTreeProc = SubclassWindow(lpDomData->hwndTreeLb, lpDomData->lpNewTreeProc);

  // pick first item id
  recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);

  // Select first item without updating right pane
  MfcBlockUpdateRightPane();
  SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)recId);
  MfcAuthorizeUpdateRightPane();

  // like for teared out doms : set the parent sub-branch if necessary
  // NEEDS TO BE DONE AFTER WE HAVE CHANGED THE CURRENT HWNDTREELB
  if (!AddParentBranch(lpDomData, recId))
    return FALSE;

  // Update caption and internal parameters
  lpDomData->domType = DOM_TYPE_REPOS;
  lstrcpy(VirtNodeName, GetVirtNodeName(lpDomData->psDomNode->nodeHandle));
  MakeDomCaption(sz, VirtNodeName, DOM_CREATE_REPOSITION, hwndMdi, TRUE);
  MfcDocSetCaption(hwndMdi, sz);
  lpDomData->domNumber = seqDomRepos;

  // Focus was missing
  SetFocus(lpDomData->hwndTreeLb);

  return TRUE;
}

// utility functions for tree resequence
// 
// technique : index in the array plus one is the new id
// search rather slow, but memory consumation low
//
static BOOL  AllocateNewRecIds(void);
static DWORD MakeNewRecId(DWORD recId);
static DWORD GetNewRecIdFromOld(DWORD refParentRecId, BOOL bAdvance);
static void  FreeNewRecIds(void);
static DWORD GetNextNewRecId();

#define MAXIMORUM 200 // maximorum 200 * 16000 approximately...
static DWORD *ReseqListAnchor[MAXIMORUM];
static int    ReseqListArrayNum;    // which array ? ( 0 to MAXIMORUM-1)
static long   ReseqListIndex;       // index in ReseqListAnchor[ReseqListArrayNum]
static long   ReseqListTotal;       // total number of elements
static long   ReseqListMaxItem;     // max number of elements in one array
static unsigned short  ReseqListSize;    // UINT TOO BIG UNDER WINDOWS NT

static BOOL  AllocateNewRecIds(void)
{

  // free previous list if forgotten
  FreeNewRecIds();

  // allocate new list space
  ReseqListSize = (unsigned short)-1;    // use unsigned trick ---> largest value
  ReseqListSize -= 10 * sizeof(DWORD);   // minus 10 items
  ReseqListSize += 1;                    // cancel the -1
  ReseqListAnchor[0] = (DWORD *)ESL_AllocMem(ReseqListSize);
  if (!ReseqListAnchor[0])
    return FALSE;
  ReseqListMaxItem = ReseqListSize / sizeof(DWORD);
  ReseqListArrayNum = 0;    // array of index 0
  ReseqListIndex = 0L;      // index in ReseqListAnchor[ReseqListArrayNum]
  ReseqListTotal = 0L;      // total number of elements
  return TRUE;
}

static void  FreeNewRecIds()
{
  int cpt;

  for (cpt = 0; cpt < MAXIMORUM; cpt++)
    if (ReseqListAnchor[cpt]) {
      ESL_FreeMem(ReseqListAnchor[cpt]);
      ReseqListAnchor[cpt] ='\0';
    }
}

static DWORD MakeNewRecId(DWORD recId)
{
  ReseqListAnchor[ReseqListArrayNum][ReseqListIndex++] = recId;
  if (ReseqListIndex < ReseqListMaxItem) {
    ReseqListTotal++;
    return ReseqListTotal;
  }
  else {
    if (ReseqListArrayNum < MAXIMORUM-1) {
      ReseqListArrayNum++;
      ReseqListIndex = 0;
      ReseqListTotal++;
      ReseqListAnchor[ReseqListArrayNum] = (DWORD *)ESL_AllocMem(ReseqListSize);
      if (ReseqListAnchor[ReseqListArrayNum])
        return ReseqListTotal;
      else {
        FreeNewRecIds();
        return 0L;
      }
    }
    else {
      FreeNewRecIds();
      return 0L;
    }
  }
}

static DWORD GetNextNewRecId()
{
  return ReseqListTotal + 1;
}

static DWORD GetNewRecIdFromOld(DWORD refParentRecId, BOOL bAdvance)
{
  long  lcpt;
  int   arrayNum;
  long  index;

  // special code for null parent
  if (refParentRecId == 0L)
    return 0L;

  for (lcpt=0L; lcpt<ReseqListTotal; lcpt++) {
    arrayNum = (int) (lcpt / ReseqListMaxItem);
    index = lcpt - arrayNum * ReseqListMaxItem;
    if (ReseqListAnchor[arrayNum][index] == refParentRecId)
      return (DWORD)(lcpt+1L);
  }
  return 0L;    // not found
}

//
//  Resequence the tree lines starting from 1
//
BOOL ResequenceTree(HWND hwndMdi, LPDOMDATA lpDomData)
{
  HWND  hwndNewTree;
  RECT  rc;               // rectangle for the new dom
  int   nanbarH;          // height of button bar
  int   statusH;          // height of status bar
  char  rsBuf[BUFSIZE];   // "resequence in progress" message

  DWORD         recId;              // record Id in original tree
  DWORD         newRecId;           // record Id in resequenced tree
  DWORD         refParentRecId;
  LPTREERECORD  lpRefData, lpData;
  LINK_RECORD   lr;
  LPSTR         lpString;
  DWORD         dwTop, dwSel;       // selection and top item

  //
  // several checks
  //
  if (!(lpDomData->bMustResequence))
    return TRUE;                // No need to resequence!
  if (lpDomData->bCreatePhase)
    return TRUE;                // in create phase
  if (DialogInProgress())
    return TRUE;                // dialog box in progress
  if (lpDomData->delayResequence > 0)
    return TRUE;                // in resequence delay state
  if (InSqlCriticalSection())
    return TRUE;                // in critical section
  if (IsBkTaskInProgress())
    return TRUE;                // bk task in progress

  // check for no lines
  if (SendMessage(lpDomData->hwndTreeLb, LM_GETCOUNT, COUNT_ALL, 0L) == 0L)
    return TRUE;

  ShowHourGlass();
  StopBkTask();

  // get mdi client rect
  GetClientRect(hwndMdi, &rc);

  // get status bar and toolbar heights
  nanbarH = GetControlsMaxHeight(lpDomData);
  statusH = (int)Status_GetHeight(lpDomData->hwndStatus);

  // adjust rectangle
  rc.top      = nanbarH;
  rc.bottom  -= statusH;

  // create Treelb window
  hwndNewTree = LRCreateWindow(hwndMdi,
    &rc,
    WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE |
    LMS_WITHICONS   | LMS_AUTOHSCROLL | LMS_ALLONITEMMESSAGES,
    DomPref.b3d);
  if (hwndNewTree == NULL) {
    RemoveHourGlass();
    ActivateBkTask();
    return FALSE;
  }

  // display message in dom status bar
  rsBuf[0] = '\0';
  LoadString(hResource, IDS_RESEQUENCING_TREE, rsBuf, sizeof(rsBuf));
  Status_SetMsg(lpDomData->hwndStatus, rsBuf);
  lpDomData->bTreeStatus = FALSE;
  if (lpDomData->bShowStatus) {
    BringWindowToTop(lpDomData->hwndStatus);
    InvalidateRect(lpDomData->hwndStatus, NULL, FALSE);
    UpdateWindow(lpDomData->hwndStatus);
  }

  // Set the font
  if (IsGDIObject(hFontBrowsers))
    SendMessage(hwndNewTree, LM_SETFONT, (WPARAM)hFontBrowsers, 0L);

  // recreate lines from the original tree
  AllocateNewRecIds();
  recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
  while (recId) {
    // get information about the line in the referred tree
    lpRefData = (LPTREERECORD)SendMessage(lpDomData->hwndTreeLb,
                                          LM_GETITEMDATA,
                                          0, (LPARAM)recId);
    if (lpRefData) {
      lpData = AllocAndFillRecord(0, FALSE, NULL, NULL, NULL, 0,
                                  NULL, NULL, NULL, NULL);  // dummy fill
        if (!lpData)
          return FALSE;   // otherwise gpf!
      _fmemcpy(lpData, lpRefData, sizeof(TREERECORD));  // real fill
    }
    else
      lpData = NULL;    // no extra data

    lpString = (LPSTR)SendMessage(lpDomData->hwndTreeLb,
                                  LM_GETSTRING, 0, (LPARAM)recId);
    refParentRecId = SendMessage(lpDomData->hwndTreeLb,
                                LM_GETPARENT, 0, (LPARAM)recId);

    // create equivalent line in the new tree
    newRecId = MakeNewRecId(recId);

    // hand create preferred to TreeAddRecord not to update currentRecId
    lr.lpString       = lpString;
    lr.ulRecID        = newRecId;
    lr.ulParentID     = GetNewRecIdFromOld(refParentRecId, FALSE);
    lr.ulInsertAfter  = 0;    // we don't care since sequential inserts
    lr.lpItemData     = lpData;

    if (!SendMessage(hwndNewTree,
                     LM_ADDRECORD,
                     0,
                     (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr)) {
      ESL_FreeMem(lpData);  // Must do since we don't call TreeAddRecord
      FreeNewRecIds();
      RemoveHourGlass();
      if (lpDomData->bShowStatus) {
        lpDomData->bTreeStatus = TRUE;
        BringWindowToTop(lpDomData->hwndTreeStatus);
        SendMessage(hwndMdi, LM_NOTIFY_ONITEM,
                    (WPARAM)lpDomData->hwndTreeLb,
                    (LPARAM)GetCurSel(lpDomData));
      }
      RemoveHourGlass();
      ActivateBkTask();
      return FALSE;
    }

    // get next
    recId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)recId);
  }

  // Second loop to set the expanded/collapsed mode for each item
  recId = SendMessage(lpDomData->hwndTreeLb, LM_GETFIRST, 0, 0L);
  while (recId) {
    if (SendMessage(lpDomData->hwndTreeLb,
                    LM_GETSTATE, 0, (LPARAM)recId) == FALSE)
      SendMessage(hwndNewTree, LM_COLLAPSE, 0,
                  (LPARAM)GetNewRecIdFromOld(recId, TRUE));
    recId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT,
                        0, (LPARAM)recId);
  }

  // store old current line and top item
  dwSel = SendMessage(lpDomData->hwndTreeLb, LM_GETSEL, 0, 0L);
  dwTop = SendMessage(lpDomData->hwndTreeLb, LM_GETTOPITEM, 0, 0L);

  // replace the old tree with the new one
  TreeDeleteAllRecords(lpDomData);
  //DestroyWindow(lpDomData->hwndTreeLb);
  lpDomData->hwndTreeLb = hwndNewTree;
  SetFocus(lpDomData->hwndTreeLb);

  // update currentRecId for next added tree lines
  lpDomData->currentRecId = GetNextNewRecId();

  // update current and top item in new tree
  // (will update the status - no need to hide "in progress" status text)
  SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0,
              (LPARAM)GetNewRecIdFromOld(dwSel, TRUE));
  SendMessage(lpDomData->hwndTreeLb, LM_SETTOPITEM, 0,
              (LPARAM)GetNewRecIdFromOld(dwTop, TRUE));

  // no more need for correspondance table
  FreeNewRecIds();

  // finished
  lpDomData->bMustResequence = FALSE;

  RemoveHourGlass();
  ActivateBkTask();
  return TRUE;
}

//
//  Update the checkmarked state of the "Show System Objects" menuitem
//  according to the current bwithsystem flag
//
static VOID NEAR UpdateShowSystemMenuItem(LPDOMDATA lpDomData)
{
  // Jan 21, 97: manage document change ---> update all menus
  if (lpDomData->bwithsystem) {
    CheckMenuItem(hGwNoneMenu, IDM_SHOW_SYSTEM   , MF_BYCOMMAND | MF_CHECKED);
  }
  else {
    CheckMenuItem(hGwNoneMenu, IDM_SHOW_SYSTEM   , MF_BYCOMMAND | MF_UNCHECKED);
  }
}

BOOL DomIsSystemChecked(HWND hWnd)
{
  HWND      hwndMdi;
  LPDOMDATA lpDomData;

  hwndMdi = GetVdbaDocumentHandle(hWnd);
  lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  return lpDomData->bwithsystem;
}

//JFS Begin

//
//  When called, the tree is assumed to have the current selection
//  on a "OT_STATIC_REPLICATOR" tree line, so we can use the current record
//  first parenthood information as the base name
//  It is also assumed the tables are not installed yet
//
static VOID NEAR InstallReplicatorTables(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  UCHAR         buf[BUFSIZE];
  UCHAR tmpusername[MAXOBJECTNAME];
  LPTREERECORD  lpRecord;
  UCHAR         objName[MAXOBJECTNAME];         // result object name
  UCHAR         bufOwner[MAXOBJECTNAME];        // result owner
  UCHAR         bufComplim[MAXOBJECTNAME];      // result complim
  UCHAR         buff [MAXOBJECTNAME];
  UCHAR         buff2 [MAXOBJECTNAME*2];
  int           resultType;
  int           resultLevel;
  int           ReplicVersion;
  
  // For UpdateDOMData
  int           iret;
  LPUCHAR       aparents[MAXPLEVEL];


  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Ask for user confirmation
  // LoadString(hResource, IDS_CONFIRM_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
  // GetWindowText(hwndMdi, buf2, sizeof(buf2)-1);
  // if (MessageBox(hwndMdi, buf, buf2, MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) != IDYES)
  // return;

      

  iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
                      lpRecord->extra,            // object name
                      "",                         // no owner name
                      OT_DATABASE,                // object type
                      0,                          // level
                      NULL,                       // aparents
                      lpDomData->bwithsystem,
                      &resultType,                // expect OT_DATABASE!
                      &resultLevel,
                      NULL,
                      objName,                    // result object name
                      bufOwner,                   // result owner
                      bufComplim);                // result complim

  if (iret != RES_SUCCESS)
    return ;

  // PS BEGIN
  {
    int      iret;
//    VNODESEL xvnode;
    REPLCONNECTPARAMS Newrepl;
	char szgateway[200];
	BOOL bHasGWSuffix;
  
    ZEROINIT (Newrepl);
//    ZEROINIT (xvnode);
    
    Newrepl.bLocalDb = TRUE;
    x_strcpy(Newrepl.szVNode,GetVirtNodeName (lpDomData->psDomNode->nodeHandle));
    x_strcpy(Newrepl.szDBName,lpRecord->extra);
    x_strcpy(Newrepl.szOwner,bufOwner);
	bHasGWSuffix = GetGWClassNameFromString(Newrepl.szVNode, szgateway);

    DBAGetUserName(Newrepl.szVNode,tmpusername);

    // Create internal table for replicator 
//   if (x_strcmp(Newrepl.szVNode,ResourceString((UINT)IDS_I_LOCALNODE)) == 0)
    if (bHasGWSuffix)
		wsprintf(buff,"repcat %s/%s -u%s",Newrepl.szDBName,szgateway,tmpusername);
	else
		wsprintf(buff,"repcat %s -u%s",Newrepl.szDBName,tmpusername);
//   else
//     wsprintf(buff,"repcat %s::%s -u%s", Newrepl.szVNode,Newrepl.szDBName,tmpusername);

    wsprintf(
      buff2,
      ResourceString ((UINT)IDS_I_INSTALL_REPLOBJ), // Installing Replication objects for %s::%s
      Newrepl.szVNode,
      Newrepl.szDBName);

    execrmcmd(Newrepl.szVNode,buff,buff2);
    SetFocus(hwndMdi);

    // Call UpdateDOMData so that the internal tables will be up-to-date
    aparents[0] = lpRecord->extra;
    aparents[1] = aparents[2] = NULL;
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_TABLE,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                         OT_VIEW,
                         1,               // level
                         aparents,
                         lpDomData->bwithsystem,
                         FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                         OT_DBEVENT,
                         1,               // level
                         aparents,
                         lpDomData->bwithsystem,
                         FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_CONNECTION,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_CDDS,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_MAILUSER,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_REGTABLE,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }

    //
    // Update the lines in the tree :
    //

    gMemErrFlag = MEMERR_NOMESSAGE;

    // 1) set cursel to the right anchor obj
    if (!GetFirstImmediateChild(lpDomData, dwCurSel, 0))
      dwCurSel = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                    0, (LPARAM)dwCurSel);
     
    // 2) simulate expand with sub-items create (force by setting wParam to 0)
    //SendMessage(hwndMdi, LM_NOTIFY_EXPAND, 0, (LPARAM)dwCurSel);

    // Make display fit to new contents of the cache
    DOMDisableDisplay   (lpDomData->psDomNode->nodeHandle, 0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_TABLE,1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_VIEW, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_DBEVENT, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CONNECTION, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CDDS, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_REGTABLE, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_MAILUSER, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
    UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, aparents[0]);
    DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);

    if (gMemErrFlag == MEMERR_HAPPENED)
      MessageBox(GetFocus(),
                ResourceString(IDS_E_MEMERR_INSTALLREPLIC),
                NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    gMemErrFlag = MEMERR_STANDARD;

    if ((ReplicVersion=GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                                lpRecord->extra, lpRecord->ownerName))
        ==REPLIC_NOINSTALL) {
      DWORD idChildObj;
      MessageBox(
        hwndMdi,
        ResourceString ((UINT) IDS_ERROR_INSTALL_REPLICATOR), // Failed to Install Replication Objects
        NULL,
        MB_OK | MB_TASKMODAL);

      // clear all children
      TreeDeleteAllChildren(lpDomData, dwCurSel);

      // rebuild dummy sub-item and collapse it
      idChildObj = AddDummySubItem(lpDomData, dwCurSel);
      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                  0, (LPARAM)dwCurSel);
      return;
    }
    if (TRUE /*vnoderet == OK*/) {
//      Newrepl.vnodelist = xvnode.vnodelist;
      x_strcpy(Newrepl.DBName,lpRecord->extra);
      Newrepl.nReplicVersion = ReplicVersion;
      iret = DlgAddReplConnection(hwndMdi, &Newrepl);
      if (!iret)  {
        DWORD idChildObj;
        LPTREERECORD lpRecord;

        // clear all children
        TreeDeleteAllChildren(lpDomData, dwCurSel);

        // rebuild dummy sub-item and collapse it
        idChildObj = AddDummySubItem(lpDomData, dwCurSel);
        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
                    0, (LPARAM)dwCurSel);

        // Mark as never expanded
        lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                              LM_GETITEMDATA,
                                              0, (LPARAM)dwCurSel);
        lpRecord->bSubValid = FALSE;
        return;
      }
      // Call UpdateDOMData so that the internal tables will be up-to-date
      aparents[0] = lpRecord->extra;
      aparents[1] = aparents[2] = NULL;
      iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                          OT_TABLE,
                          1,               // level
                          aparents,
                          lpDomData->bwithsystem,
                          FALSE);
      if (iret != RES_SUCCESS) {
        LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
        MessageBox(hwndMdi, buf, NULL,
                  MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return;
      }
      iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                          OT_VIEW,
                          1,               // level
                          aparents,
                          lpDomData->bwithsystem,
                          FALSE);
      if (iret != RES_SUCCESS) {
        LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
        MessageBox(hwndMdi, buf, NULL,
                  MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return;
      }
      iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                          OT_REPLIC_CONNECTION,
                          1,               // level
                          aparents,
                          lpDomData->bwithsystem,
                          FALSE);
      if (iret != RES_SUCCESS) {
        LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
        MessageBox(hwndMdi, buf, NULL,
                  MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return;
      }
      iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                          OT_REPLIC_MAILUSER,
                          1,               // level
                          aparents,
                          lpDomData->bwithsystem,
                          FALSE);
      if (iret != RES_SUCCESS) {
        LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
        MessageBox(hwndMdi, buf, NULL,
                  MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
        return;
      }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_REGTABLE,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                        OT_REPLIC_CDDS,
                        1,               // level
                        aparents,
                        lpDomData->bwithsystem,
                        FALSE);
    if (iret != RES_SUCCESS) {
      LoadString(hResource, IDS_ERROR_INSTALL_REPLICATOR, buf, sizeof(buf)-1);
      MessageBox(hwndMdi, buf, NULL,
                MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return;
    }
    //
    // Update the lines in the tree :
    //
      gMemErrFlag = MEMERR_NOMESSAGE;

      // 1) set cursel to the right anchor obj
      if (!GetFirstImmediateChild(lpDomData, dwCurSel, 0))
        dwCurSel = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                      0, (LPARAM)dwCurSel);
     
      // 2) simulate expand with sub-items create (force by setting wParam to 0)
      SendMessage(hwndMdi, LM_NOTIFY_EXPAND, 0, (LPARAM)dwCurSel);
      // Make display fit to new contents of the cache
      DOMDisableDisplay   (lpDomData->psDomNode->nodeHandle, 0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_TABLE,1,aparents,
                          FALSE,ACT_BKTASK,0L,0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_VIEW, 1,aparents,
                        FALSE,ACT_BKTASK,0L,0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CONNECTION, 1,aparents,
                          FALSE,ACT_BKTASK,0L,0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_MAILUSER, 1,aparents,
                          FALSE,ACT_BKTASK,0L,0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_CDDS, 1,aparents,
                          FALSE,ACT_BKTASK,0L,0);
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_REPLIC_REGTABLE, 1,aparents,
                          FALSE,ACT_BKTASK,0L,0);
      DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                  ResourceString(IDS_E_MEMERR_INSTALLREPLIC),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      gMemErrFlag = MEMERR_STANDARD;

        }
    else  {
      return;
    }
  }
  //PS END
}


static VOID NEAR ReplicatorPropagate(HWND hwndMdi, LPDOMDATA lpDomData)
{
   PROPAGATE propagate;
   LPUCHAR   vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
   DWORD         dwCurSel;
   LPTREERECORD  lpRecord;
   int           iReplicVersion;
   
   
   // initialize structure
   memset(&propagate, 0, sizeof(propagate));
   
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  iReplicVersion=GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                                        lpRecord->extra, lpRecord->ownerName);
//  if (iReplicVersion==REPLIC_V11)
//    return;

  x_strcpy(propagate.DBName,lpRecord->extra);
  propagate.iReplicType=iReplicVersion;

  SetCurReplicType(iReplicVersion);

  DlgPropagate(hwndMdi, &propagate);
}
static VOID NEAR ReplicatorBuildServer(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  UCHAR         buf[BUFSIZE];
  UCHAR         buf2[BUFSIZE];
  UCHAR         szUserName[BUFSIZE];
  LPTREERECORD  lpRecord;
  int           iReplicVersion;

  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  iReplicVersion=GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                                        lpRecord->extra, lpRecord->ownerName);
  if (iReplicVersion==REPLIC_NOINSTALL) {
    // TO BE FINISHED : SHOULD DISAPPEAR WHEN PROBLEM SOLVED IN
    // UPDATE MENU FUNCTION
    MessageBox(
       hwndMdi,
       ResourceString ((UINT)IDS_E_REPL_NOT_INSTALLED),    // Replication objects are not installed
       NULL,
       MB_OK | MB_TASKMODAL);
    return;
  }


  DBAGetUserName (GetVirtNodeName ( GetMDINodeHandle(hwndMdi)),
                  szUserName);

  switch (iReplicVersion) {
    case REPLIC_V10:
    case REPLIC_V105:
      // Ask for user confirmation
      x_strcpy(buf2,ResourceString ((UINT)IDS_I_BUILD_SERVER)); // Build Server Operation
      // Confirm Build Server Operation ?
      if (MessageBox(hwndMdi, ResourceString ((UINT)IDS_C_BUILD_SERVER), buf2,
          MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) != IDYES)
        return;
      wsprintf(buf,"ddbldsvr %s -u%s",lpRecord->extra,szUserName);
      wsprintf(
           buf2,
           ResourceString ((UINT)IDS_I_BUILD_SERVER_ON),  // Build Server Operation on  %s::%s
           GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
           lpRecord->extra);

      execrmcmd(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),buf,buf2);

      break;
    case REPLIC_V11:
      MfcDlgBuildSvr(lpDomData->psDomNode->nodeHandle,
                     GetVirtNodeName ( lpDomData->psDomNode->nodeHandle),
                     lpRecord->extra,szUserName);
      /* Old code
      iResult=DlgConfSvr(hwndMdi);
      if (iResult == 0)
        return;
      if (iResult == 1)
        wsprintf(buf,"imagerep %s -u%s",lpRecord->extra,szUserName);
      if (iResult == 2)
        wsprintf(buf,"imagerep %s -u%s -f",lpRecord->extra,szUserName);
      */
      break;
    default:
      break;
  }
}
static VOID NEAR ReplicatorReconcile(HWND hwndMdi, LPDOMDATA lpDomData)
{
   RECONCILER reconciler;
   LPUCHAR   vnodeName = GetVirtNodeName (lpDomData->psDomNode->nodeHandle);
   DWORD         dwCurSel;
   LPTREERECORD  lpRecord;
  
   
   // initialize structure
   memset(&reconciler, 0, sizeof(reconciler));
   
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

   x_strcpy(reconciler.DBName,lpRecord->extra);
   x_strcpy(reconciler.DbaName,lpRecord->ownerName);
   DlgReconciler(hwndMdi, &reconciler);

}
//JFS End

// NOTE : dereplicate is installed in $DD_INGREP/scripts

static VOID NEAR ReplicatorDereplicate(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  UCHAR         buf[BUFSIZE];
  UCHAR         buf2[BUFSIZE];
  UCHAR			tmpusername[MAXOBJECTNAME];
  UCHAR         szConnect[120];
  int           SessNum,hnode,ires;
  LPTREERECORD  lpRecord;
  int           iReplicVersion;
	char szgateway[200];
	BOOL bHasGWSuffix;

  // For UpdateDOMData
  int           iret;
  LPUCHAR       aparents[MAXPLEVEL];

  // Emb 22/3/96 : for replicator line caption update
  SETSTRING setString;
  UCHAR     bufRs[BUFSIZE];

  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  if (GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                              lpRecord->extra, lpRecord->ownerName)
      ==REPLIC_NOINSTALL) {
    // TO BE FINISHED : SHOULD DISAPPEAR WHEN PROBLEM SOLVED IN
    // UPDATE MENU FUNCTION
  
    // Replication objects are not installed
    MessageBox(hwndMdi, ResourceString ((UINT)IDS_E_REPL_NOT_INSTALLED),
               NULL, MB_OK | MB_TASKMODAL);
    return;
  }

  // Ask for user confirmation
  wsprintf(buf2, ResourceString ((UINT)IDS_I_REMOVE_REPLICATION)); // Removing replicator objects
  // Confirm Dereplicate ?
  if (MessageBox(hwndMdi, ResourceString ((UINT)IDS_C_DEREPLICATION),
      buf2, MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL) != IDYES)
    return;

  DBAGetUserName(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),tmpusername);

//  if (x_strcmp(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),ResourceString((UINT)IDS_I_LOCALNODE)) == 0)
	bHasGWSuffix = GetGWClassNameFromString(GetVirtNodeName(GetMDINodeHandle(hwndMdi)), szgateway);
	if (bHasGWSuffix)
		wsprintf(buf,"dereplic %s/%s -u%s",lpRecord->extra,szgateway, tmpusername);
	else
		wsprintf(buf,"dereplic %s -u%s",lpRecord->extra, tmpusername);
//  else
//     wsprintf(buf,"dereplic %s::%s -u%s",GetVirtNodeName(GetMDINodeHandle(hwndMdi)),lpRecord->extra);

  wsprintf(
       buf2,
       ResourceString ((UINT)IDS_I_REMOVE_REPL_FOR),    // Removing replicator objects for  %s::%s
       GetVirtNodeName(GetMDINodeHandle(hwndMdi)),
       lpRecord->extra);

  execrmcmd(GetVirtNodeName(GetMDINodeHandle(hwndMdi)),buf,buf2);

  // drop dd_server_special
  hnode = OpenNodeStruct  (GetVirtNodeName(GetMDINodeHandle(hwndMdi)));
  ires = DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
  wsprintf(szConnect,"%s::%s",GetVirtNodeName(GetMDINodeHandle(hwndMdi)),lpRecord->extra);
  Getsession(szConnect, SESSION_TYPE_INDEPENDENT, &SessNum);

  if ( DropInDD_SERVER_SPECIAL() != RES_SUCCESS)
    // MessageError(IDS_E_REPL_DROP_DD_SERVER_SPECIAL,iret);
    MessageWithHistoryButton(GetFocus(), ResourceString(IDS_E_REPL_DROP_DD_SERVER_SPECIAL));

  ReleaseSession(SessNum, RELEASE_COMMIT);
  CloseNodeStruct(hnode,FALSE);

  // Call UpdateDOMData so that the internal tables will be up-to-date
  aparents[0] = lpRecord->extra;
  aparents[1] = aparents[2] = NULL;
  iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                       OT_TABLE,
                       1,               // level
                       aparents,
                       lpDomData->bwithsystem,
                       FALSE);
  iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                       OT_VIEW,
                       1,               // level
                       aparents,
                       lpDomData->bwithsystem,
                       FALSE);
  iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                       OT_DBEVENT,
                       1,               // level
                       aparents,
                       lpDomData->bwithsystem,
                       FALSE);

  iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                       OT_PROCEDURE,
                       1,               // level
                       aparents,
                       lpDomData->bwithsystem,
                       FALSE);

  iReplicVersion=GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,
                                        lpRecord->extra, lpRecord->ownerName);

  {
    int iloc;
    int itype[]={OT_REPLIC_CONNECTION,
                 OT_REPLIC_REGTABLE,
                 OT_REPLIC_MAILUSER,
                 OT_REPLIC_CDDS };
    for (iloc=0;iloc<sizeof(itype)/sizeof(itype[0]);iloc++)
      iret = UpdateDOMData(lpDomData->psDomNode->nodeHandle,
                            itype[iloc],
                            1,               // level
                            aparents,
                            lpDomData->bwithsystem,
                            FALSE);
  }

  // error should appear in DOM tree if any.

  // Update the lines in the tree :
  // 1) set cursel to the right anchor obj
  if (!GetFirstImmediateChild(lpDomData, dwCurSel, 0))
    dwCurSel = (DWORD)SendMessage(lpDomData->hwndTreeLb, LM_GETPARENT,
                                  0, (LPARAM)dwCurSel);
     
  // 2) simulate expand with sub-items create (force by setting wParam to 0)
  SendMessage(hwndMdi, LM_NOTIFY_EXPAND, 0, (LPARAM)dwCurSel);

  // remove version number from the caption if dereplicate was successful
  if (iReplicVersion==REPLIC_NOINSTALL) {
    x_strcpy(bufRs, ResourceString(IDS_TREE_REPLICATOR_STATIC));
    setString.lpString  = bufRs;
    setString.ulRecID   = dwCurSel;
    SendMessage(lpDomData->hwndTreeLb, LM_SETSTRING, 0,
                    (LPARAM) (LPSETSTRING)&setString );
  }

  // Make display fit to new contents of the cache
  DOMDisableDisplay   (lpDomData->psDomNode->nodeHandle, 0);
  DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_TABLE,1,aparents,
                       FALSE,ACT_BKTASK,0L,0);
  DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_VIEW, 1,aparents,
                       FALSE,ACT_BKTASK,0L,0);
  DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_DBEVENT, 1,aparents,
                       FALSE,ACT_BKTASK,0L,0);
  DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, OT_PROCEDURE, 1,aparents,
                       FALSE,ACT_BKTASK,0L,0);
  UpdateDBEDisplay(lpDomData->psDomNode->nodeHandle, aparents[0]);

  // special for replicator
  {
    int iloc;
    int itype[]={OT_REPLIC_CONNECTION,
                 OT_REPLIC_REGTABLE,
                 OT_REPLIC_MAILUSER,
                 OT_REPLIC_CDDS };
    for (iloc=0;iloc<sizeof(itype)/sizeof(itype[0]);iloc++)
      DOMUpdateDisplayData(lpDomData->psDomNode->nodeHandle, itype[iloc],
                           1,aparents,
                           FALSE,ACT_BKTASK,0L,0);
  }

  DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);
}

static VOID NEAR ReplicatorMobile(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  int           iVerRepl;
  LPTREERECORD  lpRecord;
  REPMOBILE mobileparam;
  char connectname[2*MAXOBJECTNAME+2+1];
  char lpVirtNode[MAXOBJECTNAME];
  int  iret;
  int  ilocsession;

  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  iVerRepl = GetReplicInstallStatus(lpDomData->psDomNode->nodeHandle,     
                                    lpRecord->extra, lpRecord->ownerName);

  if ( iVerRepl == REPLIC_NOINSTALL ) {
    // TO BE FINISHED : SHOULD DISAPPEAR WHEN PROBLEM SOLVED IN
    // UPDATE MENU FUNCTION
  
    // Replication objects are not installed
    MessageBox(hwndMdi, ResourceString ((UINT)IDS_E_REPL_NOT_INSTALLED),
               NULL, MB_OK | MB_TASKMODAL);
    return;
  }

  x_strcpy(mobileparam.DBName,lpRecord->extra);
  
  if ( iVerRepl == REPLIC_V105 ) {
   x_strcpy(lpVirtNode,GetVirtNodeName(lpDomData->psDomNode->nodeHandle));
   wsprintf(connectname,"%s::%s",lpVirtNode,mobileparam.DBName);
   iret = Getsession(connectname,SESSION_TYPE_CACHENOREADLOCK, &ilocsession);
   
   if (iret !=RES_SUCCESS)
      return;

   iret = GetMobileInfo( &mobileparam );
   if ( iret == RES_SUCCESS )
      iret=ReleaseSession(ilocsession, RELEASE_COMMIT);
   else 
      ReleaseSession(ilocsession, RELEASE_ROLLBACK);

   DlgMobileParam(hwndMdi, &mobileparam);
  }
}


static VOID NEAR ReplicatorArcclean(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  UCHAR         szUserName[BUFSIZE];
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  char          buftime[BUFSIZE];
  int           hNode;
  char*         pVirtNodeName;
  int           ret;
   
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // call dialog defined in DgArccln.cpp
  ret = MfcDlgArcClean(buftime, sizeof(buftime));
  if (ret != IDOK)
    return;

  // Get pointer on virt node name
  hNode = lpDomData->psDomNode->nodeHandle; // or GetMDINodeHandle(hwndMdi);
  pVirtNodeName = GetVirtNodeName(hNode);

  // Get user name for current dom
  DBAGetUserName (pVirtNodeName, szUserName);

  // build and execute statement
  wsprintf(buf,"arcclean -u%s %s %s", szUserName, lpRecord->extra, buftime);
  //"Arcclean Operation on %s::%s"
  wsprintf(buf2,
           ResourceString(IDS_TITLE_ARCCLEAN_OP),
           pVirtNodeName,
           lpRecord->extra);
  execrmcmd(pVirtNodeName,buf,buf2);
}

static VOID NEAR ReplicatorRepmod(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  UCHAR         szUserName[BUFSIZE];
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  int           hNode;
  char*         pVirtNodeName;
   
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  if (GetItemObjType(lpDomData, dwCurSel) != OT_STATIC_REPLICATOR)
    return;
  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Ask for user confirmation
  //"Confirm Repmod ?" //"Repmod Operation"
  if (MessageBox(hwndMdi,
                 ResourceString(IDS_A_CONFIRM_REPMOD),
                 ResourceString(IDS_TITLE_CONFIRM_REPMOD),
                 MB_ICONQUESTION | MB_YESNO) != IDYES)
    return;

  // Get pointer on virt node name
  hNode = lpDomData->psDomNode->nodeHandle;
  pVirtNodeName = GetVirtNodeName(hNode);

  // Get user name for current dom
  DBAGetUserName (pVirtNodeName, szUserName);

  // build and execute statement
  wsprintf(buf,"repmod %s -u%s",lpRecord->extra, szUserName);
  // "Repmod Operation on %s::%s"
  wsprintf(buf2,
           ResourceString(IDS_TITLE_REPMOD_OP),
           pVirtNodeName,
           lpRecord->extra);
  execrmcmd(pVirtNodeName,buf,buf2);
}

static VOID NEAR ReplicatorCreateKeys(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  UCHAR         szUserName[BUFSIZE];
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  int           hNode,iret;
  char*         pVirtNodeName;
  int           nTableId;
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  switch (GetItemObjType(lpDomData, dwCurSel)) {
    case OTR_REPLIC_CDDS_TABLE:
    case OT_REPLIC_REGTABLE:
      break;
    default:
      assert (FALSE);
      return;
  }

  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Get pointer on virt node name
  hNode = lpDomData->psDomNode->nodeHandle;
  pVirtNodeName = GetVirtNodeName(hNode);

  // Get user name for current dom
  DBAGetUserName (pVirtNodeName, szUserName);

  // build and execute statement
  iret = MfcReplicationSelectKeyOption(lpRecord->objName);
  if (iret != NO_KEYSCHOOSE)
  {
    int ires;
    ires = GetReplicatorTableNumber (hNode ,lpRecord->extra,lpRecord->objName,lpRecord->ownerName, &nTableId);
    if (ires!=RES_SUCCESS)
    {
      //"Failure while Retrieve Table Number."
      MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_TBL_INTERNAL_NUMBER));
      return;
    }
  }
  switch (iret)
    {
    case KEYS_SHADOWTBLONLY:
      wsprintf(buf,"repcfg %s TABLE CREATEKEYS -u%s %d",lpRecord->extra,
                                                        szUserName,
                                                        nTableId);
      break ;
    case KEYS_BOTHQUEUEANDSHADOW:
      wsprintf(buf,"repcfg %s TABLE CREATEKEYS -u%s -q %d ",lpRecord->extra,
                                                            szUserName,
                                                            nTableId);
      break;
    case NO_KEYSCHOOSE:
      return;
    default:
      return;
    }
  wsprintf(buf2,
           "Creating Replication Keys %s",
           lpRecord->objName);
  execrmcmd(pVirtNodeName,buf,buf2);
}

static int SendActivateOnAllDatabases(  char *pCurVirtNodeName,char *DbName,
                                        char *pUserName,int nCddsNum)
{
  int   iret,hnodeTarget,nReplType,nCurCddsNum;
  char  szVnodeName[BUFSIZE];
  char  szDBName[BUFSIZE];
  char buff[20];
  UCHAR extradata[EXTRADATASIZE];
  LPUCHAR aparents[MAXPLEVEL];
  UCHAR bufCommand[EXTRADATASIZE];
  UCHAR bufTitle[EXTRADATASIZE];
  UCHAR DBAUsernameOntarget[MAXOBJECTNAME];

  ZEROINIT(buff);
  aparents[0]= DbName;
  _itoa(nCddsNum,buff,10);
  aparents[1]= buff;
  aparents[2]= '\0';
  for ( iret = DBAGetFirstObject (pCurVirtNodeName,
                     OT_REPLIC_CDDSDBINFO_V11,
                     2,
                     aparents,
                     TRUE,
                     szVnodeName,
                     szDBName,extradata);

       iret == RES_SUCCESS ;
       iret = DBAGetNextObject(szVnodeName,szDBName,extradata)
      )
  {
    nReplType = getint(extradata+STEPSMALLOBJ);
    nCurCddsNum = getint(extradata+(3*STEPSMALLOBJ));
    fstrncpy(DBAUsernameOntarget,extradata+(4*STEPSMALLOBJ),MAXOBJECTNAME);

    if (nCurCddsNum == nCddsNum  && nReplType == REPLIC_FULLPEER)
    {
      UCHAR szCurVnodeName[2*MAXOBJECTNAME+2];
      wsprintf(szCurVnodeName,"%s%s%s%s",szVnodeName, LPUSERPREFIXINNODENAME, DBAUsernameOntarget, LPUSERSUFFIXINNODENAME);

      hnodeTarget = OpenNodeStruct  (szCurVnodeName);
      if (hnodeTarget<0)
      {
        //"Maximum number of connections has been reached\n - Cannot Activate all Databases for this CDDS."
        MessageBox(GetFocus(),ResourceString(IDS_ERR_ACTIVATE_CDDS) ,
                  NULL, MB_ICONHAND | MB_OK | MB_TASKMODAL);
        return RES_ERR;
      }
      wsprintf(bufCommand,"repcfg %s CDDS ACTIVATE -u%s %d",szDBName,
                                  DBAUsernameOntarget,
                                  nCddsNum);
      // build and execute statement
      // "Activating Change Recording on %s::%s"
      wsprintf(bufTitle,
           ResourceString(IDS_TITLE_ACTIVE_CHANGE_RECORDING),
           szVnodeName,
           szDBName);
      execrmcmd(szCurVnodeName,bufCommand,bufTitle);
      CloseNodeStruct(hnodeTarget,FALSE);
    }
  }
  return RES_SUCCESS;
}

static VOID NEAR ReplicatorActivate(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  char          szAnswer[BUFSIZE];
  UCHAR         szUserName[BUFSIZE];
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  int           hNode;
  char*         pVirtNodeName;

  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  switch (GetItemObjType(lpDomData, dwCurSel)) {
    case OT_REPLIC_CDDS:
    case OTR_REPLIC_TABLE_CDDS:
    case OTR_REPLIC_CDDS_TABLE:
    case OT_REPLIC_REGTABLE:
      break;
    default:
      assert (FALSE);
      return;
  }

  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Get pointer on virt node name
  hNode = lpDomData->psDomNode->nodeHandle;
  pVirtNodeName = GetVirtNodeName(hNode);

  DBAGetUserName (pVirtNodeName, szUserName);

  // Ask for user confirmation
  switch (GetItemObjType(lpDomData, dwCurSel))
  {
  case OT_REPLIC_CDDS:
  case OTR_REPLIC_TABLE_CDDS:
    {
      int iret;
      //wsprintf(szAnswer,"Apply also to all full peer databases of CDDS");
      wsprintf(buf,"repcfg %s CDDS ACTIVATE -u%s %d",lpRecord->extra,
                                                     szUserName,
                                                     lpRecord->complimValue);
      iret = MessageBox(hwndMdi,ResourceString(IDS_A_APPLY_ALL_FULL_PEER),
                        ResourceString(IDS_TITLE_ACTIVATE_RECORDING),
                        MB_ICONQUESTION | MB_YESNOCANCEL );
      if (iret == IDCANCEL)
        return;
      if (iret == IDYES)
      {
        SendActivateOnAllDatabases( pVirtNodeName,lpRecord->extra,
                                    szUserName,lpRecord->complimValue);
        return;
      }
    }
    break;
  case OT_REPLIC_REGTABLE:
  case OTR_REPLIC_CDDS_TABLE:
    {
      int ires,nTableId;
      ires = GetReplicatorTableNumber (hNode ,lpRecord->extra,lpRecord->objName,lpRecord->ownerName, &nTableId);
      if (ires!=RES_SUCCESS)
      {
        MessageWithHistoryButton(GetFocus(), ResourceString(IDS_ERR_TBL_INTERNAL_NUMBER));
        return;
      }
      //"Activate Change Recording on Table %s ?"
      //"Activating Change Recording"
      wsprintf(szAnswer,ResourceString(IDS_AF_ACTIVATE_RECORDING),lpRecord->objName);
      wsprintf(buf, "ddgenrul %s %d -u%s",lpRecord->extra,nTableId,szUserName);
      if (MessageBox(hwndMdi,
                     szAnswer,
                     ResourceString(IDS_TITLE_ACTIVATE_RECORDING),
                     MB_ICONQUESTION | MB_YESNO) != IDYES)
        return;
    }
    break;
  default:
    return;
  }

  // build and execute statement
  //"Activating Change Recording on %s::%s"
  wsprintf(buf2,
           ResourceString(IDS_TITLE_ACTIVE_CHANGE_RECORDING),
           pVirtNodeName,
           lpRecord->extra);
  execrmcmd(pVirtNodeName,buf,buf2);
}

static VOID NEAR ReplicatorDeactivate(HWND hwndMdi, LPDOMDATA lpDomData)
{
  DWORD         dwCurSel;
  LPTREERECORD  lpRecord;
  char          szAnswer[BUFSIZE];
  UCHAR         szUserName[BUFSIZE];
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  int           hNode;
  char*         pVirtNodeName;
  int           nObjType;
  // Double-check the assumptions.
  dwCurSel = GetCurSel(lpDomData);
  switch (GetItemObjType(lpDomData, dwCurSel)) {
    case OT_REPLIC_CDDS:
    case OTR_REPLIC_TABLE_CDDS:
    case OTR_REPLIC_CDDS_TABLE:
    case OT_REPLIC_REGTABLE:
      break;
    default:
      assert (FALSE);
      return;
  }

  lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                            LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  // Get pointer on virt node name
  hNode = lpDomData->psDomNode->nodeHandle;
  pVirtNodeName = GetVirtNodeName(hNode);

  DBAGetUserName (pVirtNodeName, szUserName);
  nObjType = GetItemObjType(lpDomData, dwCurSel);
  // Ask for user confirmation
  switch (nObjType)
  {
  case OT_REPLIC_CDDS:
  case OTR_REPLIC_TABLE_CDDS:
    //"Deactivate Change Recording for Cdds Number %d ?"
    wsprintf(szAnswer,ResourceString(IDS_AF_DEACTIVATE_CDDS),lpRecord->complimValue);
    wsprintf(buf,"repcfg %s CDDS DEACTIVATE -u%s %d",lpRecord->extra,
                                                     szUserName,
                                                     lpRecord->complimValue);
    break;
  case OT_REPLIC_REGTABLE:
  case OTR_REPLIC_CDDS_TABLE:
    // "Deactivate Change Recording for Table %s ?"
    wsprintf(szAnswer,ResourceString(IDS_AF_DEACTIVATE_TABLE),lpRecord->objName);
    break;
  default:
    return;
  }
  //"Deactivating Change Recording"
  if (MessageBox(hwndMdi,
                 szAnswer,
                 ResourceString(IDS_TITLE_DEACTIVATING),
                 MB_ICONQUESTION | MB_YESNO) != IDYES)
    return;
  if (nObjType == OT_REPLIC_REGTABLE || nObjType == OTR_REPLIC_CDDS_TABLE)
  {
    int ret,iret,SessNum;
    char szConnect[MAXOBJECTNAME];
    ret = RES_ERR;
    iret = RES_ERR;
    wsprintf(szConnect,"%s::%s",pVirtNodeName,lpRecord->extra);
    Getsession(szConnect, SESSION_TYPE_CACHENOREADLOCK, &SessNum);
    ret = ForceFlush( StringWithoutOwner (lpRecord->objName), lpRecord->ownerName );
    if (ret != RES_ERR) {
      wsprintf(buf,"UPDATE dd_regist_tables SET rules_created = '' "
                   "WHERE table_name = '%s' AND table_owner='%s'",StringWithoutOwner (lpRecord->objName),
                                                                  lpRecord->ownerName);
      iret=ExecSQLImm(buf,FALSE, NULL, NULL, NULL);
    }
    if ( iret == RES_SUCCESS && ret == RES_SUCCESS)
       ReleaseSession(SessNum, RELEASE_COMMIT);
    else 
       ReleaseSession(SessNum, RELEASE_ROLLBACK);
  }
  else
  {
    // build and execute statement
    //"Deactivating Change Recording on %s::%s"
    wsprintf(buf2,
             ResourceString(IDS_TITLE_DEACTIVATING_CHANGE_REC),
             pVirtNodeName,
             lpRecord->extra);
    execrmcmd(pVirtNodeName,buf,buf2);
  }
}


//
// Calls the dialog box that allows to select the object types to be
// refreshed, and refreshes the trees
//
static BOOL  NEAR RefreshTrees(HWND hwndMdi, LPDOMDATA lpDomData)
{
  int   ret;

  DWORD dwCurSel = GetCurSel(lpDomData);
  int   CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
  BOOL  bCurSelStatic = IsCurSelObjStatic(lpDomData);

  // Specific for 2 pane doc
  BOOL bHasProperties;
  int CurItemBasicType = GetBasicType(CurItemObjType);
  LPTREERECORD lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                     LM_GETITEMDATA, 0, (LPARAM)dwCurSel);

  
  // properties pane - different from HasProperties4Display() since new types
  bHasProperties = HasPropertiesPane(CurItemBasicType);
  // Manage "< No xyz >" and other < Txt > cases
  if (lpRecord->objName[0] == '\0')
    bHasProperties = FALSE;

  UpdateIceDisplayBranch(hwndMdi, lpDomData);

    gMemErrFlag = MEMERR_NOMESSAGE;
    DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, 0);
    ret = DOMUpdateDisplayData2 (
            lpDomData->psDomNode->nodeHandle, // hnodestruct
            OT_VIRTNODE,                      // iobjecttype
            0,                                // level
            NULL,                             // parentstrings
            FALSE,                            // bWithChildren
            ACT_NO_USE,                       // not concerned :
                                              // forceRefresh is the key
            0L,                       // no more used, all branches refreshed
                                      // for type combination
                                      // LONG/DWORD size identity assumed!
            hwndMdi,                          // current for UpdateDomData,
                                              // then all DOM MDI windows
                                              // on this node for display
            FORCE_REFRESH_ALLBRANCHES);       // Mode requested for refresh
    DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, 0);

    // Fix Nov 5, 97: must Update number of objects after a force refresh
    // (necessary since we have optimized the function that counts the tree items
    // for performance purposes)
    GetNumberOfObjects(lpDomData, TRUE);
    // display will be updated at next call to the same function

    if (gMemErrFlag == MEMERR_HAPPENED)
      MessageBox(GetFocus(),
                ResourceString(IDS_E_MEMERR_FORCEREFRESH),
                NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    gMemErrFlag = MEMERR_STANDARD;

  if (bHasProperties) {
    BOOL bUpdate = TRUE;

    // Emb March 30, 98: current selection may have changed,
    // which may lead to crash if the item has disappeared
    // ---> get all parameters again...
    DWORD dwNewCurSel = GetCurSel(lpDomData);
    if (dwNewCurSel != dwCurSel) {
      dwCurSel = dwNewCurSel;
      CurItemObjType = GetItemObjType(lpDomData, dwCurSel);
      CurItemBasicType = GetBasicType(CurItemObjType);
      bCurSelStatic = IsCurSelObjStatic(lpDomData);

      bHasProperties = HasPropertiesPane(CurItemBasicType);
      // Manage "< No xyz >" and other < Txt > cases
      // NEED TO GET THE 'FRESH' LPRECORD BEFORE TESTING OBJNAME!!!
      lpRecord = (LPTREERECORD) SendMessage(lpDomData->hwndTreeLb,
                                                       LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
      if (lpRecord->objName[0] == '\0')
        bHasProperties = FALSE;
    }

    TS_MfcUpdateRightPane(hwndMdi, lpDomData, bCurSelStatic, dwCurSel, CurItemBasicType,
                       lpRecord, bHasProperties, bUpdate, 0, FALSE);
  }

  return TRUE;

}


//
//  Dialog proc for the box that manages the DOM filters
//
BOOL WINAPI __export DomFiltersDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg) 
  {
    HANDLE_MSG(hDlg, WM_INITDIALOG, FilterOnInitDialog);
    HANDLE_MSG(hDlg, WM_COMMAND, FilterOnCommand);
    HANDLE_MSG(hDlg, WM_DESTROY, FilterOnDestroy);
  }
  return FALSE;
}


static BOOL FilterOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    LPDOMDATA lpDomData;
    HWND      hCtl;

    if (!HDLGSetProp(hDlg,(LPVOID) lParam))
        EndDialog(hDlg,FALSE);
    CenterDialog(hDlg);
    lpDomData = (LPDOMDATA)lParam;

    // Fill the combos and put selection on them
      hCtl = GetDlgItem(hDlg, IDD_DOMFILTER_BASE);
      FillCombo(lpDomData, hCtl, TRUE);
      SetComboSelection(hCtl, lpDomData->lpBaseOwner);
    hCtl = GetDlgItem(hDlg, IDD_DOMFILTER_OTHER);
    FillCombo(lpDomData, hCtl, TRUE);
    SetComboSelection(hCtl, lpDomData->lpOtherOwner);

    lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)DOMFILTERSDIALOG));

    return TRUE;
}

static void FilterOnDestroy (HWND hDlg)
{
   lpHelpStack = StackObject_POP (lpHelpStack);
}

static void FilterOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
  LPDOMDATA lpDomData;
  HWND      hCtl;
  char      buf[BUFSIZE];
  char      buf2[BUFSIZE];
  DWORD     dwIndex;
  BOOL      bBaseStateChange, bOtherStateChange;
  HWND      hwndMdi;          // current MDI Doc

  switch (id) 
  {
    case IDOK:
      lpDomData = (LPDOMDATA)HDLGGetProp(hDlg);

      lpDomData->delayResequence++;

      bBaseStateChange = FALSE;
      bOtherStateChange = FALSE;

        // get current state (current selection) for base owner
        hCtl = GetDlgItem(hDlg, IDD_DOMFILTER_BASE);
        dwIndex = SendMessage(hCtl, CB_GETCURSEL, 0, 0L);
        if (dwIndex != CB_ERR)
          SendMessage(hCtl, CB_GETLBTEXT, (WPARAM)dwIndex,
                      (LPARAM)(LPCSTR)buf);
        else
          buf[0] = '\0';
        // compare states
        if (x_strcmp(lpDomData->lpBaseOwner, buf) != 0) {
          // state has changed : update the information in lpDomData
          // and in the combobox
          bBaseStateChange = TRUE;
          LoadString(hResource, IDS_FILTER_OWNER_ALL, buf2, sizeof(buf2));
          if (x_strcmp(buf, buf2))
            lstrcpy(lpDomData->lpBaseOwner, buf);
          else
            lpDomData->lpBaseOwner[0] = '\0';   // no filter
          FillCombo(lpDomData, lpDomData->hwndBaseOwner, TRUE); // Force fill
          SetComboSelection(lpDomData->hwndBaseOwner, lpDomData->lpBaseOwner);
        }

      // get current state (current selection) for other owner
      hCtl = GetDlgItem(hDlg, IDD_DOMFILTER_OTHER);
      dwIndex = SendMessage(hCtl, CB_GETCURSEL, 0, 0L);
      if (dwIndex != CB_ERR)
        SendMessage(hCtl, CB_GETLBTEXT, (WPARAM)dwIndex,
                    (LPARAM)(LPCSTR)buf);
      else
        buf[0] = '\0';
      // compare states
      if (x_strcmp(lpDomData->lpOtherOwner, buf) != 0) {
        // state has changed : update the information in lpDomData
        // and in the combobox
        bOtherStateChange = TRUE;
        LoadString(hResource, IDS_FILTER_OWNER_ALL, buf2, sizeof(buf2));
        if (x_strcmp(buf, buf2))
          lstrcpy(lpDomData->lpOtherOwner, buf);
        else
          lpDomData->lpOtherOwner[0] = '\0';   // no filter
        FillCombo(lpDomData, lpDomData->hwndOtherOwner, TRUE); // Force fill
        SetComboSelection(lpDomData->hwndOtherOwner, lpDomData->lpOtherOwner);
      }

      if (bBaseStateChange || bOtherStateChange) {
        // State has changed : update immediately the tree
        hwndMdi = (HWND)SendMessage(hwndMDIClient,
                                    WM_MDIGETACTIVE, 0, 0L);

        gMemErrFlag = MEMERR_NOMESSAGE;
        DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        if (bBaseStateChange)
          DOMUpdateDisplayData (
                  lpDomData->psDomNode->nodeHandle, // hnodestruct
                  OT_VIRTNODE,                      // iobjecttype
                  0,                                // level
                  NULL,                             // parentstrings
                  FALSE,                            // bWithChildren
                  ACT_CHANGE_BASE_FILTER,           // other filter changed
                  0L,                               // no item id
                  GetVdbaDocumentHandle(hwndMdi));  // current mdi window
        if (bOtherStateChange)
          DOMUpdateDisplayData (
                  lpDomData->psDomNode->nodeHandle, // hnodestruct
                  OT_VIRTNODE,                      // iobjecttype
                  0,                                // level
                  NULL,                             // parentstrings
                  FALSE,                            // bWithChildren
                  ACT_CHANGE_OTHER_FILTER,          // other filter changed
                  0L,                               // no item id
                  GetVdbaDocumentHandle(hwndMdi));  // current mdi window
        DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        if (gMemErrFlag == MEMERR_HAPPENED)
          MessageBox(GetFocus(),
                    ResourceString(IDS_E_MEMERR_FILTERCHANGE),
                    NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        gMemErrFlag = MEMERR_STANDARD;
      }

      lpDomData->delayResequence--;

      HDLGRemoveProp(hDlg);
      EndDialog(hDlg, TRUE);
      break;

    case IDCANCEL:
      HDLGRemoveProp(hDlg);
      EndDialog(hDlg, FALSE);
      break;

    default:
      break;
  }
}

//
//  Manage the dialog box to choose the filters
//
static VOID NEAR ManageFiltersBox(HWND hwndMdi, LPDOMDATA lpDomData)
{
  int     retval;
  DLGPROC lpfnDlg;

  lpfnDlg = (DLGPROC) MakeProcInstance((FARPROC)DomFiltersDlgProc, hInst);
  retval = VdbaDialogBoxParam(hResource,
                            MAKEINTRESOURCE(DOMFILTERSDIALOG),
                            hwndMdi,
                            lpfnDlg,
                            (LPARAM)lpDomData);
  FreeProcInstance ((FARPROC) lpfnDlg);
  // NOTE : new setting already applied to current dom on OK
}


//------------------------------------------------------------------------
//
// Toolbar special objects management (comboboxes and checkboxes)
//
//------------------------------------------------------------------------

LONG FAR PASCAL __export BaseOwnerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LONG FAR PASCAL __export OtherOwnerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LONG FAR PASCAL __export SystemProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//
// ManageNotifMsg :
//
// Manage the notification messages coming from the controls on bar
//
// Return value: TRUE if concerned with, FALSE if not.
//
static BOOL NEAR ManageNotifMsg(HWND hwndMdi, LPDOMDATA lpDomData, int id, HWND hwndCtl, UINT codeNotify)
{
  int   checkedState;
  char  buf[BUFSIZE];
  char  buf2[BUFSIZE];
  DWORD dwIndex;

  // SEEN UNDER DEBUGGER : wParam is NULL for the notification messages
  // THIS MIGHT NOT ALWAYS BE THE CASE...
  if (id)
    return FALSE;   // not concerned with

  // manage BaseOwner combo box
  if (hwndCtl == lpDomData->hwndBaseOwner) {
    if (codeNotify == CBN_DROPDOWN) {
      // fill the list on dropdown only
      SetExpandingCombobox();
      FillCombo(lpDomData, hwndCtl, TRUE);
      ResetExpandingCombobox();
      SetComboSelection(hwndCtl, lpDomData->lpBaseOwner);
      return TRUE;
    }
    if (codeNotify == CBN_SELCHANGE) {
      // get current state (current selection)
      dwIndex = SendMessage(hwndCtl, CB_GETCURSEL, 0, 0L);
      if (dwIndex != CB_ERR)
        SendMessage(hwndCtl, CB_GETLBTEXT, (WPARAM)dwIndex,
                    (LPARAM)(LPCSTR)buf);
      else
        buf[0] = '\0';
      // compare states
      if (x_strcmp(lpDomData->lpBaseOwner, buf) != 0) {
        // state has changed : update the information in lpDomData
        LoadString(hResource, IDS_FILTER_OWNER_ALL, buf2, sizeof(buf2));
        if (x_strcmp(buf, buf2))
          lstrcpy(lpDomData->lpBaseOwner, buf);
        else
          lpDomData->lpBaseOwner[0] = '\0';   // no filter

        // State has changed : update immediately the tree
        lpDomData->delayResequence++;
        gMemErrFlag = MEMERR_NOMESSAGE;
        DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        DOMUpdateDisplayData (
                lpDomData->psDomNode->nodeHandle, // hnodestruct
                OT_VIRTNODE,                      // iobjecttype
                0,                                // level
                NULL,                             // parentstrings
                FALSE,                            // bWithChildren
                ACT_CHANGE_BASE_FILTER,           // base filter changed
                0L,                               // no item id
                hwndMdi);                         // current mdi window
        DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        if (gMemErrFlag == MEMERR_HAPPENED)
          MessageBox(GetFocus(),
                    ResourceString(IDS_E_MEMERR_BASEFILTERCHANGE),
                    NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        gMemErrFlag = MEMERR_STANDARD;
        lpDomData->delayResequence--;
      }
    }
  }

  // manage OtherOwner combo box
  if (hwndCtl == lpDomData->hwndOtherOwner) {
    if (codeNotify == CBN_DROPDOWN) {
      // fill the list on dropdown only
      SetExpandingCombobox();
      FillCombo(lpDomData, hwndCtl, TRUE);
      ResetExpandingCombobox();
      SetComboSelection(hwndCtl, lpDomData->lpOtherOwner);
      return TRUE;
    }
    if (codeNotify == CBN_SELCHANGE) {
      // get current state (current selection)
      dwIndex = SendMessage(hwndCtl, CB_GETCURSEL, 0, 0L);
      if (dwIndex != CB_ERR)
        SendMessage(hwndCtl, CB_GETLBTEXT, (WPARAM)dwIndex,
                    (LPARAM)(LPCSTR)buf);
      else
        buf[0] = '\0';
      // compare states
      if (x_strcmp(lpDomData->lpOtherOwner, buf) != 0) {
        // state has changed : update the information in lpDomData
        LoadString(hResource, IDS_FILTER_OWNER_ALL, buf2, sizeof(buf2));
        if (x_strcmp(buf, buf2))
          lstrcpy(lpDomData->lpOtherOwner, buf);
        else
          lpDomData->lpOtherOwner[0] = '\0';   // no filter

        // State has changed : update immediately the tree
        lpDomData->delayResequence++;
        gMemErrFlag = MEMERR_NOMESSAGE;
        DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        DOMUpdateDisplayData (
                lpDomData->psDomNode->nodeHandle, // hnodestruct
                OT_VIRTNODE,                      // iobjecttype
                0,                                // level
                NULL,                             // parentstrings
                FALSE,                            // bWithChildren
                ACT_CHANGE_OTHER_FILTER,          // other filter changed
                0L,                               // no item id
                hwndMdi);                         // current mdi window
        DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
        if (gMemErrFlag == MEMERR_HAPPENED)
          MessageBox(GetFocus(),
                    ResourceString(IDS_E_MEMERR_OTHERFILTERCHANGE),
                    NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        gMemErrFlag = MEMERR_STANDARD;
        lpDomData->delayResequence--;
      }
    }
  }

  // manage checkbox state change
  if (hwndCtl == lpDomData->hwndSystem) {
    checkedState = (int)SendMessage(hwndCtl, BM_GETCHECK, 0, 0L);
    if ((BOOL)checkedState != lpDomData->bwithsystem) {
      // state has changed : update information in lpDomData
      lpDomData->bwithsystem = (BOOL)checkedState;

      // State has changed : update menuitem (checkmark)
      UpdateShowSystemMenuItem(lpDomData);

      // State has changed : check owner filters validity
      if (lpDomData->bwithsystem == FALSE) {
        if (IsSystemObject(OT_USER, lpDomData->lpBaseOwner, "")) {
          lpDomData->lpBaseOwner[0] = '\0';   // no filter
          SetComboSelection(lpDomData->hwndBaseOwner, lpDomData->lpBaseOwner);
        }
        if (IsSystemObject(OT_USER, lpDomData->lpOtherOwner, "")) {
          lpDomData->lpOtherOwner[0] = '\0';   // no filter
          SetComboSelection(lpDomData->hwndOtherOwner, lpDomData->lpOtherOwner);
        }
      }

      // State has changed : update immediately the tree
      lpDomData->delayResequence++;
      gMemErrFlag = MEMERR_NOMESSAGE;
      DOMDisableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
      DOMUpdateDisplayData (
              lpDomData->psDomNode->nodeHandle, // hnodestruct
              OT_VIRTNODE,                      // iobjecttype
              0,                                // level
              NULL,                             // parentstrings
              FALSE,                            // bWithChildren
              ACT_CHANGE_SYSTEMOBJECTS,         // systemobjects flag
              0L,                               // no item id
              hwndMdi);                         // current mdi window
      DOMEnableDisplay(lpDomData->psDomNode->nodeHandle, hwndMdi);
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                  ResourceString(IDS_E_MEMERR_SHOWSYSTEM),
                  NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      gMemErrFlag = MEMERR_STANDARD;
      lpDomData->delayResequence--;
    }
  }

  return FALSE;   // not concerned with
}


LRESULT FAR PASCAL __export BaseOwnerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPDOMDATA     lpDomData;
  static long   lastMousePos;
  lpDomData = (LPDOMDATA)GetWindowLong(hwnd, GWL_USERDATA);

  switch (message) {

    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
      if (wParam==VK_RETURN || wParam==VK_ESCAPE)
        SetFocus(lpDomData->hwndTreeLb);
      if (wParam==VK_TAB)
        SetFocus(lpDomData->hwndOtherOwner);
      break;
  }
  return (LONG)CallWindowProc(lpDomData->lpOldBaseOwnerProc, hwnd, message, wParam, lParam);
}


LONG FAR PASCAL __export OtherOwnerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPDOMDATA     lpDomData;
  static long   lastMousePos;

  lpDomData = (LPDOMDATA)GetWindowLong(hwnd, GWL_USERDATA);

  switch (message) {

    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
      if (wParam==VK_RETURN || wParam==VK_ESCAPE)
        SetFocus(lpDomData->hwndTreeLb);
      if (wParam==VK_TAB)
        SetFocus(lpDomData->hwndSystem);
      break;
  }
  return (LONG)CallWindowProc((WNDPROC)lpDomData->lpOldOtherOwnerProc, hwnd, message, wParam, lParam);
}


LONG FAR PASCAL __export SystemProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPDOMDATA     lpDomData;
  static long   lastMousePos;

  lpDomData = (LPDOMDATA)GetWindowLong(hwnd, GWL_USERDATA);
  switch (message) {

    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
      if (wParam==VK_RETURN || wParam==VK_ESCAPE)
        SetFocus(lpDomData->hwndTreeLb);
      if (wParam==VK_TAB)
        SetFocus(lpDomData->hwndTreeLb);
      break;
  }
  return (LONG)CallWindowProc(lpDomData->lpOldSystemProc, hwnd, message, wParam, lParam);
}


static VOID NEAR DeleteNanBarSpecialObjects(LPDOMDATA lpDomData)
{
    SetWindowLong(lpDomData->hwndBaseOwner, GWL_WNDPROC,
                  (LPARAM)(WNDPROC) lpDomData->lpOldBaseOwnerProc);
    DestroyWindow(lpDomData->hwndBaseOwner);
    FreeProcInstance(lpDomData->lpNewBaseOwnerProc);

  SetWindowLong(lpDomData->hwndOtherOwner, GWL_WNDPROC,
                (LPARAM)(WNDPROC) lpDomData->lpOldOtherOwnerProc);
  DestroyWindow(lpDomData->hwndOtherOwner);
  FreeProcInstance(lpDomData->lpNewOtherOwnerProc);

  SetWindowLong(lpDomData->hwndSystem, GWL_WNDPROC,
                (LPARAM)(WNDPROC) lpDomData->lpOldSystemProc);
  DestroyWindow(lpDomData->hwndSystem);
  FreeProcInstance(lpDomData->lpNewSystemProc);
}


static void SubclassCombo(HWND hwndCombo, LPDOMDATA lpDomData)
{
  if (hwndCombo) {
    // subclass WndProc
    if (hwndCombo == lpDomData->hwndBaseOwner) {
        lpDomData->lpNewBaseOwnerProc= (WNDPROC)MakeProcInstance((FARPROC)BaseOwnerProc, hInst);
        lpDomData->lpOldBaseOwnerProc = (WNDPROC)GetWindowLong(hwndCombo, GWL_WNDPROC);
        SetWindowLong(hwndCombo, GWL_WNDPROC, (LONG)lpDomData->lpNewBaseOwnerProc);
    }
    else {
      lpDomData->lpNewOtherOwnerProc= (WNDPROC)MakeProcInstance((FARPROC)OtherOwnerProc, hInst);
      lpDomData->lpOldOtherOwnerProc = (WNDPROC)GetWindowLong(hwndCombo, GWL_WNDPROC);
      SetWindowLong(hwndCombo, GWL_WNDPROC, (LONG)lpDomData->lpNewOtherOwnerProc);
    }
    // store lpDomData
    SetWindowLong(hwndCombo, GWL_USERDATA, (LONG)lpDomData);
  }
}

static void SubclassCheckbox(HWND hwndCheck, LPDOMDATA lpDomData)
{
  if (hwndCheck) {
    // subclass WndProc
    lpDomData->lpNewSystemProc= (WNDPROC)MakeProcInstance((FARPROC)SystemProc, hInst);
    lpDomData->lpOldSystemProc = (WNDPROC)GetWindowLong(hwndCheck, GWL_WNDPROC);
    SetWindowLong(hwndCheck, GWL_WNDPROC, (LONG)lpDomData->lpNewSystemProc);
    // store lpDomData
    SetWindowLong(hwndCheck, GWL_USERDATA, (LONG)lpDomData);
  }
}

static void UnSubclassAllToolbarControls(LPDOMDATA lpDomData)
{
    SetWindowLong(lpDomData->hwndBaseOwner, GWL_WNDPROC,
                  (LPARAM)(WNDPROC) lpDomData->lpOldBaseOwnerProc);
    FreeProcInstance(lpDomData->lpNewBaseOwnerProc);

  SetWindowLong(lpDomData->hwndOtherOwner, GWL_WNDPROC,
                (LPARAM)(WNDPROC) lpDomData->lpOldOtherOwnerProc);
  FreeProcInstance(lpDomData->lpNewOtherOwnerProc);

  SetWindowLong(lpDomData->hwndSystem, GWL_WNDPROC,
                (LPARAM)(WNDPROC) lpDomData->lpOldSystemProc);
  FreeProcInstance(lpDomData->lpNewSystemProc);
}


static BOOL NEAR FillCombo(LPDOMDATA lpDomData, HWND hwndCombo, BOOL bForce)
{
  int     iret;
  UCHAR   userName[MAXOBJECTNAME];
  UCHAR   userOwner[MAXOBJECTNAME];       // not used
  UCHAR   userComplim[MAXOBJECTNAME];     // not used
  DWORD   dwIndex;
  BOOL    bRet;

  // First Sequence of DOMGetFirst/Next before empty list,
  // so that potential response time delay is consumed here
  // only if bForce is set
  if (bForce) {
    ShowHourGlass();
    iret =  DOMGetFirstObject(lpDomData->psDomNode->nodeHandle, // hnodestruct
                              OT_USER,                // iobjecttype
                              0,                      // level
                              NULL,                   // aparents
                              lpDomData->bwithsystem, // bwithsystem
                              NULL,                   // lpfilterowner
                              userName,               // object name
                              userOwner,              // object's owner
                              userComplim);           // complimentary data
    // loop on users names
    while (iret == RES_SUCCESS)
      iret = DOMGetNextObject(userName, userOwner, userComplim);
    RemoveHourGlass();
  }

  // empty list
  SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0L);

  // fill with users list only if bForce is not set
  if (bForce) {
    // get first user name
    iret =  DOMGetFirstObject(lpDomData->psDomNode->nodeHandle, // hnodestruct
                              OT_USER,                // iobjecttype
                              0,                      // level
                              NULL,                   // aparents
                              lpDomData->bwithsystem, // bwithsystem
                              NULL,                   // lpfilterowner
                              userName,               // object name
                              userOwner,              // object's owner
                              userComplim);           // complimentary data
    // loop on users names
    while (iret != RES_ENDOFDATA) {
      if (iret==RES_ERR || iret==RES_TIMEOUT) {
        // string <error>
        LoadString(hResource, IDS_FILTER_OWNER_ERROR, userName, sizeof(userName));
        dwIndex = SendMessage(hwndCombo, CB_ADDSTRING,
                              0, (LPARAM)(LPCSTR)userName);
        if (dwIndex==CB_ERR || dwIndex==CB_ERRSPACE)
          bRet = FALSE;
        else
          bRet = TRUE;
        break;
      }
      if (iret==RES_NOGRANT) {
        // string <error>
        LoadString(hResource, IDS_FILTER_OWNER_NOGRANT,
                  userName, sizeof(userName));
        dwIndex = SendMessage(hwndCombo, CB_ADDSTRING,
                              0, (LPARAM)(LPCSTR)userName);
        if (dwIndex==CB_ERR || dwIndex==CB_ERRSPACE)
          bRet = FALSE;
        else
          bRet = TRUE;
        break;
      }

      // add item except if "(public)"
      if (x_strcmp(userName, lppublicdispstring())) {
        dwIndex = SendMessage(hwndCombo, CB_ADDSTRING,
                              0, (LPARAM)(LPCSTR)userName);
        if (dwIndex==CB_ERR || dwIndex==CB_ERRSPACE) {
          bRet = FALSE;
          break;
        }
      }

      // next object
      iret = DOMGetNextObject(userName, userOwner, userComplim);
    }
  }

  // whatever bForce value is :
  // initial string <all users> put at first place, with selection on it
  LoadString(hResource, IDS_FILTER_OWNER_ALL, userName, sizeof(userName));
  dwIndex = SendMessage(hwndCombo, CB_INSERTSTRING,
                        0, (LPARAM)(LPCSTR)userName);
  if (dwIndex==CB_ERR || dwIndex==CB_ERRSPACE)
    bRet = FALSE;
  else
    bRet = TRUE;
  return bRet;
}

static VOID NEAR SetComboSelection(HWND hwndCombo, UCHAR *filter)
{
  DWORD dwIndex;
  UCHAR noFilter[MAXOBJECTNAME];
  UCHAR *locFilter;

  // search for <all owners> if no filter given
  if (filter == NULL || filter[0] == '\0') {
    LoadString(hResource, IDS_FILTER_OWNER_ALL, noFilter, sizeof(noFilter));
    locFilter = noFilter;
  }
  else
    locFilter = filter;
  dwIndex = SendMessage(hwndCombo, CB_FINDSTRINGEXACT,
                        (WPARAM)-1, (LPARAM)(LPCSTR)locFilter);
  if (dwIndex != CB_ERR)
    SendMessage(hwndCombo, CB_SETCURSEL, (WPARAM) dwIndex, 0L);
}

static VOID NEAR SetCheckboxState(HWND hwndCheck, BOOL bChecked)
{
  SendMessage(hwndCheck, BM_SETCHECK, (WPARAM)(bChecked?1:0), 0L);
}


//--------------------------------------------------------------------------
//
// TreeStatus bar functions
//
// This section is derived from rich williams sqlact.c and sqlact.h sources
// (ribbon functions)
//
//--------------------------------------------------------------------------

// Structure describing the tree status bar

// the strings in resource are only used to compute the display width
// of each item of the tree status bar - The maximum text width of each
// item is given in the next to last column of the TreeStatusBarData array

// Width of objname can reach 4*MAXOBJECTNAME plus the []
#define OBJNAMEWIDTH (MAXOBJECTNAME + 3*(MAXOBJECTNAME+2))

static DOMTREESTATUSBAR TreeStatusBarData[] =
{
  RBTID_OBJTYPE      , IDS_TS_W_OBJTYPE      , SBT_NORMAL  | SBT_VISIBLE, COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_LEFT   | DT_NOPREFIX, 100,            RB_SMALL_GAP,
  RBTID_OBJNAME      , IDS_TS_W_OBJNAME      , SBT_VISIBLE | SBT_3D,      COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_LEFT   | DT_NOPREFIX, OBJNAMEWIDTH,   RB_LARGE_GAP,
  RBTID_CAPT_OWNER   , IDS_TS_W_CAPT_OWNER   , SBT_NORMAL  | SBT_VISIBLE, COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX, 50,             RB_SMALL_GAP,
  RBTID_OWNERNAME    , IDS_TS_W_OWNERNAME    , SBT_VISIBLE | SBT_3D,      COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_LEFT   | DT_NOPREFIX, MAXOBJECTNAME,  RB_LARGE_GAP,
  RBTID_CAPT_COMPLIM , IDS_TS_W_CAPT_COMPLIM , SBT_NORMAL  | SBT_VISIBLE, COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX, 100,            RB_SMALL_GAP,
  RBTID_COMPLIM      , IDS_TS_W_COMPLIM      , SBT_VISIBLE | SBT_3D,      COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_LEFT   | DT_NOPREFIX, MAXOBJECTNAME,  RB_LARGE_GAP,
  RBTID_BSYSTEM      , IDS_TS_W_BSYSTEM      , SBT_VISIBLE | SBT_3D,      COLOR_BTNTEXT, COLOR_BTNFACE, DT_VCENTER | DT_SINGLELINE | DT_CENTER | DT_NOPREFIX, 50,             RB_SMALL_GAP,
  -1
};

//
//    Creates and initializes the tree status bar (bar underneath the nanbar).
//
//  Parameters:
//    hwnd  - Handle to the mdi window
//
//  Returns:
//    The handle to the tree status bar window if successful, otherwise NULL.
//
HWND CreateTreeStatusBarWnd(HWND hwndMdi)
{
  LPSB_DEFINITION lpStatBarDef;
  HWND hwndSB;
  int i;
  HFONT hFont;
  LPDOMDATA lpDomData;
  char      szField[BUFSIZE];

  if ((hFont = MakeTreeStatusBarFont(hwndMdi)) == NULL)
    hFont = GetStockObject( SYSTEM_FONT );

  lpDomData = (LPDOMDATA)GetWindowLong(hwndMdi, 0);
  lpDomData->hTreeStatusFont = hFont;

  // Emb replaces GlobalAllocPtr(LHND, size) by ESL_AllocMem(size)
  lpStatBarDef = (LPSB_DEFINITION)
      ESL_AllocMem( sizeof(SB_DEFINITION) +
                    sizeof(SB_FIELDDEF) * RB_DOM_NFIELDS );
  if (lpStatBarDef == NULL)
    return NULL;

  /* Initial size and location of status bar */

  lpStatBarDef->x = 0;
  lpStatBarDef->y = 0;
  lpStatBarDef->cx = 0;
  lpStatBarDef->cy = 0;
  lpStatBarDef->lFrameColor = GetSysColor(COLOR_WINDOWFRAME);
  lpStatBarDef->lLightColor = RGB( 255, 255, 255 );
  lpStatBarDef->lShadowColor = RGB( 127, 127, 127 );
  lpStatBarDef->lBkgnColor = GetSysColor( COLOR_BTNFACE );
  lpStatBarDef->nFields = RB_DOM_NFIELDS;

  i = 0;

  while (TreeStatusBarData[i].nID != -1)
  {
    lpStatBarDef->fields[i].wID = (WORD)TreeStatusBarData[i].nID;
    lpStatBarDef->fields[i].wType = TreeStatusBarData[i].wType;
    lpStatBarDef->fields[i].lTextColor = GetSysColor(TreeStatusBarData[i].nSysTextColor);
    lpStatBarDef->fields[i].lFaceColor = GetSysColor(TreeStatusBarData[i].nSysFaceColor);
    lpStatBarDef->fields[i].wFormat = TreeStatusBarData[i].wFormat;
    lpStatBarDef->fields[i].wLength = TreeStatusBarData[i].wLength;

    SetRect( &lpStatBarDef->fields[i].boundRect, 0, 0, 0, 0 );
    lpStatBarDef->fields[i].hFont = hFont;

    i++;
  }

  hwndSB = NewStatusBar( hInst, hwndMdi, lpStatBarDef );

  // Emb replaces GlobalFreePtr(lpStatBarDef) by ESL_FreeMem(lpStatBarDef)
   ESL_FreeMem( lpStatBarDef );

  lpDomData->bInitTreeStatusFieldSizes = TRUE;

  /* Initialize label fields */
  if (hwndSB)
  {
    i = 0;

    while (TreeStatusBarData[i].nID != -1)
    {

      if (TreeStatusBarData[i].nStringId != -1)
      {
        LoadString(hResource, TreeStatusBarData[i].nStringId, szField, sizeof(szField));

        SendMessage( hwndSB, SBM_SETFIELD, TreeStatusBarData[i].nID, (LPARAM)szField);
      }

      i++;
    }

    ShowWindow( hwndSB, SW_SHOW );
  }

  return hwndSB;
}

//
//  Return the font handle that will be used for the status bar window
//
static HFONT NEAR MakeTreeStatusBarFont(HWND hwnd)
{
  return hFontStatus;   // Font used for all status bars, defined in main.c
}

// Changed Emb : only initializes the fields sizes (if necessary)
static void NEAR SizeTreeStatusBarWnd(HWND hwndParent)
{
  LPDOMDATA lpDomData;
  int x;
  int nLength, cyBar, cyField, yField;
  TEXTMETRIC tm;
  char szBuf[BUFSIZE];
  RECT rect;
  HDC hdc;
  int i;

  lpDomData = (LPDOMDATA)GetWindowLong(hwndParent, 0);

  hdc = GetDC(lpDomData->hwndTreeStatus);

  SelectObject( hdc, lpDomData->hTreeStatusFont );

  GetTextMetrics( hdc, &tm );
  cyField = tm.tmHeight + (2 * RB_VERT_SPACER);
  cyBar = cyField + RB_TOP_MARGIN + RB_BOTTOM_MARGIN;
  yField = ((cyBar - cyField) / 2);

  if (lpDomData->bInitTreeStatusFieldSizes)
  {
    i = 0;
    x = RB_LEFT_MARGIN;

    // Only do this on the first size message

    while (TreeStatusBarData[i].nID != -1)
    {
      if (TreeStatusBarData[i].nStringId != -1)
        LoadString(hResource, TreeStatusBarData[i].nStringId, szBuf, sizeof(szBuf));
      else
        lstrcpy(szBuf, "000000");    // size for a numeric field

      nLength = LOWORD(GetTextExtent(hdc, szBuf, lstrlen(szBuf)));

      rect.left = x;
      rect.top = yField;
      rect.right = x + nLength;
      rect.bottom = rect.top + cyField;

      SendMessage( lpDomData->hwndTreeStatus, SBM_SIZEFIELD, TreeStatusBarData[i].nID, (LPARAM)(LPRECT)&rect);

      x = rect.right + TreeStatusBarData[i].nGap;

      i++;
    }

    lpDomData->bInitTreeStatusFieldSizes = FALSE;

    // draw with empty fields
    SendMessage(hwndParent, LM_NOTIFY_ONITEM, 0, 0L);
  }

  ReleaseDC( lpDomData->hwndTreeStatus, hdc );
}

//
//  Calculates the height of the tree status bar in pixels.
//
//  Parameters:
//    hwnd  - Handle to the tree status bar
//
//  Returns:
//    The height of the tree status bar in pixels.
//
//  Modified Emb so it can be called at any time,
//  especially before the tree status has been resized
//
//  Uses the lpDomData->hTreeStatusFont to calculate the height
//
static int NEAR GetTreeStatusBarHeight(HWND hwnd)
{
  LPDOMDATA lpDomData;
  int cyBar, cyField;
  TEXTMETRIC tm;
  HDC hdc;

  lpDomData = (LPDOMDATA)GetWindowLong(GetParent(hwnd), 0);

  hdc = GetDC(lpDomData->hwndTreeStatus);

  SelectObject( hdc, lpDomData->hTreeStatusFont);

  GetTextMetrics( hdc, &tm );
  cyField = tm.tmHeight + (2 * RB_VERT_SPACER);
  cyBar = cyField + RB_TOP_MARGIN + RB_BOTTOM_MARGIN;

  // DON'T FORGET! OTHERWISE THE APP LOCKS AFTER SOME DOM RESIZE OPERATIONS
  ReleaseDC( lpDomData->hwndTreeStatus, hdc );

  return cyBar;
}

//
// VOID DomWindowChangePreferences(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
//
// manages WM_USER_CHANGE_PREFERENCE on the Dom Window
//
static VOID NEAR DomWindowChangePreferences(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam)
{
  RECT  rc;
  int   typeDoc;

  typeDoc = lpDomData->typeDoc;
  lpDomData->bShowNanBar = QueryShowNanBarSetting(typeDoc);
  lpDomData->bShowStatus = QueryShowStatusSetting(typeDoc);

  if (!IsIconic(hwndMdi)) {
    GetClientRect(hwndMdi, &rc);
    DomWindowSize(hwndMdi,
                       lpDomData,
                       SIZE_RESTORED ,  // any but SIZE_MINIMIZED
                       rc.right-rc.left, rc.bottom-rc.top);
  }
}

// Adds a static sub item, itself having optionnaly a dummy sub item
// OBSOLETE : extern DWORD AddStaticSubItem(LPDOMDATA lpDomData, DWORD idParent, UCHAR *bufParent1, UCHAR *bufParent2, UCHAR *bufParent3, UCHAR *bufOwner, UCHAR *bufComplim, UCHAR *tableSchema, int otStatic, UINT idRsString, BOOL bWithDummy, BOOL bSpeedFlag);
// 

//
// Manage the "locate" menuitem
//
static BOOL NEAR ManageLocate(HWND hwndMdi, LPDOMDATA lpDomData)
{
  LOCATEPARAMS  loc;
  BOOL          bRet;
  int           level;            // level of parenthood
  BOOL          bNoWild;          // no wildcard saying "all" ?
  char          rsBuf[BUFSIZE];   // ressource string
  char          rsCapt[BUFSIZE];  // ressource string

  LPUCHAR       aparents[MAXPLEVEL];      // parenthood
  UCHAR         bufPar0[MAXOBJECTNAME];   // parent level 0
  UCHAR         bufPar1[MAXOBJECTNAME];   // parent level 1
  UCHAR         bufPar2[MAXOBJECTNAME];   // parent level 2

  // result data (call to DOMGetObject)
  LPUCHAR       aparentsResult[MAXPLEVEL];      // result aparents
  UCHAR         bufParResult0[MAXOBJECTNAME];   // result parent level 0
  UCHAR         bufParResult1[MAXOBJECTNAME];   // result parent level 1
  UCHAR         bufParResult2[MAXOBJECTNAME];   // result parent level 2
  UCHAR         objName[MAXOBJECTNAME];         // result object name
  UCHAR         bufOwner[MAXOBJECTNAME];        // result owner
  UCHAR         bufComplim[MAXOBJECTNAME];      // result complim
//  int           resultType;
//  int           resultLevel;

  // tree line
  DWORD         idChildObj;       // child object id
//  DWORD         idChild2Obj;      // sub-child object id
  DWORD         idLast;           // last in the tree, for insert after
//  int           iret;
  LPTREERECORD  lpRecord;

  // for static branch
  int           staticType;       // type of static branch to be created
  UINT          idsStatic;        // string id for static branch
//  BOOL          bNoDummy;         // No dummy sub item to be added

  int           dbType;           // for parent database star type propagation

  // call dialog box
  bRet = DlgLocate(hwndMdi, &loc);
  if (!bRet)
    return FALSE;

  // anchor aparents on buffers
  aparents[0] = bufPar0;
  aparents[1] = bufPar1;
  aparents[2] = bufPar2;

  // anchor aparentsResult on buffers
  aparentsResult[0] = bufParResult0;
  aparentsResult[1] = bufParResult1;
  aparentsResult[2] = bufParResult2;


  // Special management for security alarms
  if (loc.ObjectType == OTLL_SECURITYALARM) {
    // TO BE FINISHED : INSERT OBJECT IN CURRENT DOM
    MessageBox(hwndMdi, "Security alarm : To be finished", NULL,
               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
  }

  // parenthood level
  level = GetParentLevelFromObjType(loc.ObjectType);

  // Wildcard says "all" ? - TO BE FINISHED : STRING FROM VUT SAYING "All"
  LoadString(hResource, (UINT)IDS_I_LOCATE_ALL, rsBuf, sizeof(rsBuf));
//  if (lstrcmp(loc.FindString, rsBuf))
//    bNoWild = TRUE;
//  else
    bNoWild = FALSE;


  //
  // Get full info for branch to be created, and create the branch
  //
//  if (bNoWild) {
//
//    // prepare parenthood information
//    bufPar0[0] = bufPar1[0] = bufPar2[0] = '\0';
//    if (level > 0)
//      x_strcpy(bufPar0, loc.DBName);
//
//    // clear result buffers and call DOMGetObject
//    bufParResult0[0] = bufParResult1[0] = bufParResult2[0] = '\0';
//    objName[0] = bufOwner[0] = '\0';
//    bufComplim[0] = '\0';   // TO BE FINISHED : REMOVE WHEN MERGE FNN
//    iret = DOMGetObject(lpDomData->psDomNode->nodeHandle,
//                        loc.FindString,         // object name
//                        "",                     // TO BE FINISHED
//                        loc.ObjectType,         // object type
//                        level,                  // level
//                        aparents,               // known aparents
//                        lpDomData->bwithsystem,
//                        &resultType,
//                        &resultLevel,
//                        aparentsResult, // array of result parent strings
//                        objName,        // result object name
//                        bufOwner,       // result owner
//                        bufComplim);    // result complimentary data
//    // clear bufOwner and bufComplim AFTER call - TO BE FINISHED
//    if (resultType!= OT_DATABASE && !NonDBObjectHasOwner(resultType))
//      bufOwner[0] = '\0';
//
//    // check object existence
//    if (iret == RES_ENDOFDATA) {
//      LoadString(hResource, IDS_LOCATE_ERR_NOOBJ, rsBuf, sizeof(rsBuf));
//      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//      return FALSE;
//    }
//
//    // create the tree branch
//    lpRecord = AllocAndFillRecord(resultType, FALSE,
//                                  aparentsResult[0],
//                                  aparentsResult[1],
//                                  aparentsResult[2],
//                                  objName, bufOwner, bufComplim,
//                                  NULL);    // SCHEMA ???
//    if (!lpRecord) {
//      LoadString(hResource, IDS_LOCATE_ERR_MEMORY, rsBuf, sizeof(rsBuf));
//      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//      return FALSE;
//    }
//    idLast = GetLastRecordId(lpDomData);
//    idChildObj = TreeAddRecord(lpDomData, objName, 0, idLast, 0, lpRecord);
//    if (idChildObj==0) {
//      LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
//      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//      return FALSE;
//    }
//
//    // check against system flag
//    bNoDummy = FALSE;
//    if (lpDomData->bwithsystem == FALSE)
//      if (IsSystemObject(GetBasicType(resultType), objName, bufOwner))
//        bNoDummy = TRUE;
//
//    // check against database filter
//    if (bNoDummy == FALSE)
//      if (resultType == OT_DATABASE && lpDomData->lpBaseOwner[0])
//        if (lstrcmp(bufOwner, lpDomData->lpBaseOwner))
//          bNoDummy = TRUE;
//
//    // check against other filter
//    if (bNoDummy == FALSE)
//      if ( NonDBObjectHasOwner(resultType) && lpDomData->lpOtherOwner[0])
//        if (lstrcmp(bufOwner, lpDomData->lpOtherOwner))
//          bNoDummy = TRUE;
//
//    // finish if not expandable
//    if (bNoDummy) {
//      // Set the selection on the item
//      SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)idChildObj);
//
//      // Finished
//      return TRUE;
//    }
//
//    // special management according to object type
//    switch (resultType) {
//      case OT_INTEGRITY:
//        // store integrity number with getint
//        lpRecord->complimValue = (LONG)getint(bufComplim);
//        lpRecord->szComplim[0] = bufComplim[0] = '\0';
//        bNoDummy = TRUE;
//        break;
//
//      case OT_PROCEDURE:
//        // add static sub-items
//        idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
//                      aparentsResult[0],
//                      objName,
//                      NULL,
//                      bufOwner, bufComplim, NULL,
//                      OT_STATIC_PROCGRANT_EXEC_USER,
//                      IDS_TREE_PROCGRANT_EXEC_USER_STATIC, TRUE);
//        if (idChild2Obj==0) {
//          LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
//          LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//          MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//          return FALSE;
//        }
//        idChild2Obj = AddStaticSubItem(lpDomData, idChildObj,
//                      aparentsResult[0],
//                      objName,
//                      NULL,
//                      bufOwner, bufComplim, NULL,
//                      OT_STATIC_R_PROC_RULE,
//                      IDS_TREE_R_PROC_RULE_STATIC, TRUE);
//        if (idChild2Obj==0) {
//          LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
//          LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//          MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//          return FALSE;
//        }
//        // Set the item in collapsed mode
//        SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE,
//                    0, (LPARAM)idChildObj);
//        bNoDummy = TRUE;
//        break;
//    }
//
//    // Add dummy sub-sub-item except exceptions
//    if (!bNoDummy) {
//      if (AddDummySubItem(lpDomData, idChildObj)==0) {
//        LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
//        LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
//        MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
//        return FALSE;
//      }
//      // Set the sub item in collapsed mode
//      SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idChildObj);
//    }
//
//    // Set the selection on the item
//    SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)idChildObj);
//
//    // add parent branches for the inserted object
//    if (!AddParentBranch(lpDomData, idChildObj))
//      return FALSE;
//
//    // if only parent branch, say subbranch valid
//    if (bNoDummy)
//      lpRecord->bSubValid = TRUE;
//
//    // OK!
//    return TRUE;
//  }
//  else {

    // Restricted features if gateway
    if (GetOIVers() == OIVERS_NOTOI) {
      switch (loc.ObjectType) {
        case OT_DATABASE:
        case OT_TABLE:
        case OT_VIEW:
        case OT_USER:
          break;  // accepted
        default:
          // "Selected object type not available for this node type"
          MessageBox(hwndMdi,
                     ResourceString(IDS_ERR_SELECTED_OBJECT_TYPE),
                     NULL,
                     MB_ICONEXCLAMATION | MB_OK);
          return FALSE;
      }
    }

    // check level
    if (level > 1) {
      LoadString(hResource, IDS_LOCATE_ERR_WILD_UNACC, rsBuf, sizeof(rsBuf));
      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
    }

    // build data for tree branch
    staticType = GetStaticType(loc.ObjectType);
    idsStatic  = GetStaticStringId(staticType);
    LoadString(hResource, idsStatic, objName, sizeof(objName));

    // prepare parenthood information
    bufPar0[0] = bufPar1[0] = bufPar2[0] = '\0';
    if (level > 0)
      x_strcpy(bufPar0, loc.DBName);
    bufOwner[0] = bufComplim[0] = '\0';

    x_strcat(objName," ");
    fstrncpy(objName+x_strlen(objName),
             loc.FindString,
             MAXOBJECTNAME-x_strlen(objName));
    if (level > 0)
      dbType = GetDatabaseStarType(lpDomData->psDomNode->nodeHandle, loc.DBName);
    else
      dbType = 0;

    // create the tree branch
    lpRecord = AllocAndFillRecord(staticType, FALSE,
                                  bufPar0, bufPar1, bufPar2, dbType,
                                  objName, bufOwner, bufComplim,
                                  NULL);    // SCHEMA ???
    if (!lpRecord) {
      LoadString(hResource, IDS_LOCATE_ERR_MEMORY, rsBuf, sizeof(rsBuf));
      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
    }
    fstrncpy(lpRecord->wildcardFilter, loc.FindString, MAXOBJECTNAME);

    idLast = GetLastRecordId(lpDomData);
    idChildObj = TreeAddRecord(lpDomData, objName, 0, idLast, 0, lpRecord);
    if (idChildObj==0) {
      LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
    }

    // Add dummy sub-sub-item (always for static item)
    if (AddDummySubItem(lpDomData, idChildObj)==0) {
      LoadString(hResource, IDS_LOCATE_ERR_ADDLINE, rsBuf, sizeof(rsBuf));
      LoadString(hResource, IDS_LOCATE_ERR_CAPT,  rsCapt, sizeof(rsCapt));
      MessageBox(GetFocus(), rsBuf, rsCapt, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      return FALSE;
    }

    // Set the sub item in collapsed mode
    // TO BE FINISHED : ONLY IF MEETS FILTER AND SYSTEM ATTRIBUTES
    SendMessage(lpDomData->hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)idChildObj);

    // Set the selection on the item
    SendMessage(lpDomData->hwndTreeLb, LM_SETSEL, 0, (LPARAM)idChildObj);

    // add parent branches for the static
    if (!AddParentBranch(lpDomData, idChildObj))
      return FALSE;

    // OK!
    return TRUE;
//  }

  return TRUE;
}

LRESULT WINAPI EXPORT TreeSubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_CHAR, TreeOnChar);
    }

    return CommonTreeSubclassDefWndProc(hwnd, message, wParam, lParam);
}


LRESULT CommonTreeSubclassDefWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(GetParent(hwnd), 0);
    if (!lpDomData)
      return 0;

    return CallWindowProc(lpDomData->lpOldTreeProc, hwnd, message, wParam, lParam);
}


void TreeOnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    switch (ch)
    {
        case '/':
        {
            DWORD dwCurSel=(DWORD)SendMessage(hwnd, LM_GETSEL, 0, 0L);
            if (dwCurSel)
            {
                SendMessage(hwnd, LM_COLLAPSEBRANCH, 0, (LPARAM)dwCurSel);
            }
            return;
        }
    }

    FORWARD_WM_CHAR(hwnd, ch, cRepeat, CommonTreeSubclassDefWndProc);
}


// Added Emb Dec 16, 96 : launch open-ingres from inside vdba
#ifdef WIN32
#ifdef WINSTART_INCLUDED

long LaunchIngstartDialogBox(HWND hWnd)
{
  TCHAR tchszCommand [BUFSIZE];
  BOOL bSuccess;
  STARTUPINFO StartupInfo;
  LPSTARTUPINFO lpStartupInfo;
  PROCESS_INFORMATION ProcInfo;
  char szWinstartExe[BUFSIZE];
  HCURSOR   hOldCursor  = NULL;
  HCURSOR   hWaitCursor = LoadCursor(NULL, IDC_WAIT);
  lstrcpy (tchszCommand, VDBA_GetWinstart_CmdName());
  lpStartupInfo = &StartupInfo;
  memset(lpStartupInfo, 0, sizeof(STARTUPINFO));
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_SHOWDEFAULT;
  x_strcpy (szWinstartExe, tchszCommand);

  bSuccess = CreateProcess(
      NULL,            // pointer to name of executable module
      szWinstartExe,   // pointer to command line string
      NULL,            // pointer to process security attributes
      NULL,            // pointer to thread security attributes
      FALSE,           // handle inheritance flag
      0,               // creation flags
      NULL,            // pointer to new environment block
      NULL,            // pointer to current directory name
      lpStartupInfo,   // pointer to STARTUPINFO
      &ProcInfo        // pointer to PROCESS_INFORMATION
      );
  if (!bSuccess) {
    char buf0[200];
    //"Failure in starting the %s installation from VisualDba"
    wsprintf(buf0,ResourceString(IDS_F_STARTING_INSTALL),
                  VDBA_GetInstallName4Messages());
    MessageBox (hWnd, buf0, NULL, MB_OK | MB_TASKMODAL);
    return -1L;
  }

  // loop til' process terminated
  // Ps/Emb : let the app repaint itself while starting Ingres
  hOldCursor = SetCursor(hWaitCursor);
  while(1) {
    DWORD exitCode;
    MSG message;

	// 08-Nov-99 (noifr01) bug 99437: reenabled Sleep(100) otherwise VDBA uses 100% CPU.
	// 100 ms seems a convenient compromise that allows the "Paint" not to be slow, and seems
	// enough for using a neglectible CPU time percentage
    Sleep(100);       // sleep for 100 milliseconds

    if (!GetExitCodeProcess(ProcInfo.hProcess, &exitCode)) {
      SetCursor(hOldCursor);
      return -1L;     // failed
    }
    if (exitCode != STILL_ACTIVE) {
      SetCursor(hOldCursor);
      return exitCode;
    }

    SetCursor(hWaitCursor);
    if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
      SetCursor(hWaitCursor);
      if (message.message == WM_PAINT) {
        TranslateMessage (&message);
        DispatchMessage  (&message);
      }
    }
    else
      SetCursor(hWaitCursor);
  }

  // should never execute here!
  SetCursor(hOldCursor);
  return 0L;
}

#endif  // WINSTART_INCLUDED
#endif  // WIN32



