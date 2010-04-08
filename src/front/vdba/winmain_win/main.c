/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

//#define DEBUG_CONTEXT_HELP
//#define DEBUGHELP
//#define NOCRITICALSECTION
#define CRITICALONLYREACTIVATE

//#define NOLOADSAVE  // temporary for unix version

// Select window when double clicking in mdiclient, or not?
#ifdef MAINWIN
#define NOSELWIN
#endif

/*
**
**  Project : CA/OpenIngres Visual DBA
**
**  Source : main.c
**
**  Description:
**	creates the application and the frame window
**	manages the WM_COMMAND messages
**
**  Author : Emmanuel Blattes
**
**  History (after 01-01-2000):
**	19-Jan-2000 (noifr01)
**	    (bug 100065) get the extension of VDBA saved files from a
**	    (unique) resource in a resource file.
**	    moved an erorr message to a resource, and have the extension
**	    of vdba saved files become a substitution variable within that
**	    message
**	19-Jan-2000 (noifr01)
**	    (bug 100068) 
**	    the old code that was adding the default extension to the
**	    environment file to be loaded upon startup, if it contains no
**	    "." character, was still remaining at a place where it now
**	    doesn't apply any more, since the VDBA 2.x architecture change,
**	    where whe "winmain" logic got encapsulated in a library called
**	    from the new mainmfc directory code. moved it (and adapted it)
**	    to the new right place in the same source
**	20-Jan-2000 (uk$so01)
**	    Bug #100063
**	    Eliminate the undesired compiler's warning
**	25-Jan-2000 (noifr01)
**	    (bug #100104) provide a warning, when changing the "activate
**	    background refresh" setting when VDBA is invoked "in the context"
**	    by the network utility, mentioning that this is a global setting
**	    that will affect later independent VDBA sessions
**	27-Jan-2000
**	    (SIR #100175)
**	    Call function MSGContinueBox (LPCTSTR lpszMessage) instead of
**	    MessageBox();
**	16-feb-2000 (hanch04, cucjo01, somsa01)
**	    Generisized include of saveload.h . Also, removed use of
**	    MSGContinueBox().
**	24-feb-2000 (somsa01)
**	    The previous change accidentally removed the changes for bug
**	    100104 and SIR 100175. I have placed that code back.
**	20-Jul-2000 (hanch04)
**	    Use PCcmdline for system calls instead of fork and exec.
**  04-Jul-2000 (noifr01)
**      (bug 102276) #ifdef'ed MAINWIN those changes (from 20-Jul-2000
**      above) that weren't yet #ifdef'ed. This allows to get rid of 
**      new compile warnings that appear now under NT because of this
**      change, and fixes the fact that VDBA was not compiling any more
**      under NT (this could have been fixed by defining NT_GENERIC for
**      this file in the .dsp file used to build under NT, but the
**      current change, done initially for removing the warnings, fixes 
**      that)
**  25-Sep-2000 (noifr01)
**     (bug 99242) clean-up for DBCS compliance
**  20-Dec-2000 (noifr01)
**   (SIR 103548) removed obsolete function call that was there for speedup
**    reasons. No more needed because of the implementation of the SIR.
**  22-Dec-2000 (noifr01)
**   (SIR 103548) updated the way to get the application title
**  26-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
**  30-Mar-2001 (noifr01)
**   (sir 99596) removal of references to obsolete DLL's
**  30-Mar-2001 (noifr01)
**   (sir 104378) differentiation of II 2.6.
**  18-Oct-2001 (schph01)
**   (bug 106086) used GetVdbaHelpStatus() function to verify if the display
**   of vdba.hlp is needed, when the populate dialog box is launch since vdba
**   only the iia.hlp file is displayed.
**  10-Dec-2001 (noifr01)
**   (sir 99596) removal of obsolete code and resources
** 04-Fev-2002 (uk$so01)
**    SIR #106648, Integrate (ipm && sqlquery).ocx.
**    Save properties ad default.
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Integrate SQL Assistant In-Process Server
** 28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
** 04-Jun-2002 (uk$so01)
**    BUG #105480 Prohibit the MessageBox of background refresh to come up
**    in the foreground window if VDBA is not in the foreground window.
** 05-Aug-2003 (uk$so01)
**    SIR #106648, Remove printer setting
** 24-Nov-2003 (schph01)
**    (SIR 107122) Change Ingres Version initialization (OIVERS_26 by OIVERS_265)
** 09-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 23-Jan-2004 (schph01)
**    (sir 104378) detect version 3 of Ingres, for managing
**    new features provided in this version. replaced references
**    to 2.65 with refereces to 3  in #define definitions for
**    better readability in the future
** 27-Jan-2004 (schph01)
**    (sir 104378) Remove IDS_VERSION_ABOUT, since the about box
**    is now implemented according to the corporate guidelines
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 02-Feb-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Eliminate the warning.
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
** 04-Jun-2004 (uk$so01)
**    SIR #111701, Connect Help to History of SQL Statement error.
**    and change the dompref helpid from DOMPREFDIALOG to 5501 (domprefs_box)
** 30-Nov-2004 (uk$so01)
**    BUG #113544 / ISSUE #13821971: help content menu should always display the Overview
** 09-Mar-2010 (drivi01)
**    SIR #123397
**    Turn off the prompt to save the window environment on exit.
*/

//
// Includes
//
//#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>
#include <htmlhelp.h>

// esql and so forth management
#include "dba.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "error.h"
#include "dbafile.h"
#include "treelb.e"     // tree listbox dll
#include "main.h"
#include "dom.h"
#include "resource.h"
#include "winutils.h"
#include "nanact.e"   // nanbar and status bar dll
#include "extccall.h" // New Preference Dialog


#ifdef MAINWIN
# include <compat.h>
# include <gl.h>
# include <lo.h>
# include <st.h>
# include <pc.h>
#endif

#ifdef NOSELWIN
#define MdiSelWinInit(dummy)
#define MdiSelWinEnd(dummy)
#define MdiSelWinBox(dummy)
#else
#include "selwin.e"   // "More windows" box dll
#endif

#include "tree.h"
#include "settings.h" // bk task enabled/disabled state

#include "dbadlg1.h"  // dialog boxes part one
#ifdef _USE_CTL3D
#include "ctl3d.h"    // 3d dialog boxes
#endif


#include "esltools.h"   // Timer

// for print functions in print.c
#include "typedefs.h"
#include "stddlg.e"
#include "print.e"    // PrintSetup

//
// for global status bar functions
//
#include "statbar.h"

// special for error messages on box "load environment - check password"
#include "msghandl.h"
#include "dll.h"

// C/C++ interface for custom message box with button to launch errors history
#include "dgerrh.h"

// C/C++ interface for product name
#include "prodname.h"

// for dialogs ids used by context help system
#include "dlgres.h"
/* // */
/* // UK.S 11-AUG-1997: MAINWIN */
/* // */
#if defined (MAINWIN)
#include <unistd.h>
#endif

extern BOOL IsSavePropertyAsDefault();

// from mainfrm.cpp
extern void MainFrmUpdateStatusBar(int nbConnSrv, char *srvName, char *bufNbObjects, char *bufDate, char *bufTime);
extern int  MfcSaveSettings(FILEIDENT idFile);
extern int  MfcLoadSettings(FILEIDENT idFile);
extern void MfcResetNodesTree();
extern void SetOpenEnvSuccessFlag();
extern BOOL IsNodesWindowVisible();
extern HWND GetMfcNodesToolbarDialogHandle();

// from UpdDisp.cpp
extern void UpdateIpmDocsFont(HFONT hFontIpmTree);
extern void UpdateNodeFont(HFONT hFontNodeTree);

// from IpmFrame.cpp
extern int  MfcGetNumberOfIpmObjects(HWND hwndDoc);

#include <saveload.h>

// locally defined
void ManageNodesPreferences(HWND hwndParent);

// context help ids
// defined in mainmfc.h #define HELPID_MONITOR      8004
#define HELPID_DBEVENT      8005
#define HELPID_NODESEL      8006
#define HELPID_BLANKSCREEN  8007

extern DWORD GetDomDocHelpId(HWND curDocHwnd);      // defaults to HELPID_DOM 
extern DWORD GetMonitorDocHelpId(HWND curDocHwnd);  // defaults to HELPID_MONITOR
static DWORD GetDbeventDocHelpId(HWND curDocHwnd) { return HELPID_DBEVENT; }
static DWORD GetNodeselDocHelpId(HWND curDocHwnd) { return HELPID_NODESEL; }


//
//  End of section for tree status bar functions
//

//
// global variables
//
HINSTANCE hInst;                  // Program instance handle
HINSTANCE hResource;              // Resource handle
HWND      hwndFrame     = NULL;   // Handle to main window
HWND      hwndMDIClient = NULL;   // Handle to MDI client
HWND      hwndToolBar;            // Handle to global tool bar
UINT      uNanNotifyMsg;          // Message used to notify toolbar owner
BOOL      bSaveCompressed;        // Save files using the pkware compression
BOOL      bReadCompressed;

// Multi-menu management added July 26, 96
HMENU     hGwNoneMenu;            // when doc on Ingres node (no gateway)

//
// Vut Adds 39-30 Aug-1995
//  

LPSTACKOBJECT  lpHelpStack = NULL; // Stack of active dlg box.
HHOOK          hHookFilter;
HOOKPROC       lpHookProcInstance;

LRESULT CALLBACK HookFilterFunc (int nCode, WPARAM wParam, LPARAM lParam);
void           MainOnF1Down (int nCode);
DWORD          dwMenuHelpId;    // Emb help id for menuitems
#define        OFFSETHELPMENU 20000 

//
// Vut Ends
//  


// double-type status bar management
HWND    hwndStatusBar;          // Handle to global status bar
BOOL    bGlobalStatus;          // TRUE if GlobalStatus in use, FALSE if help text
HWND    hwndGlobalStatus;             // Handle to the GlobalStatus bar window
BOOL    bInitGlobalStatusFieldSizes;  // Must the fields be sized?

HFONT   hFontStatus;            // Font used for all status bars
HFONT   hFontBrowsers;          // Font used for all browsers
HFONT   hFontPropContainers;    // Font used for all properties containers

HFONT  hFontIpmTree;         // Font for all monitor trees
HFONT  hFontNodeTree;        // Font for the node tree

// modeless dialog boxes
HWND   hDlgRefresh;      // refresh dialog box
HWND   hDlgErrorsTrack;  // Error track dialog box

// static variables
//static char       szHelpFile[] = "vdba.hlp";
static char       szFrame[] = "ingres_dbatool_frame"; // frame class name

// 'Find' item
HWND   hDlgSearch = NULL;     // handle on "find" modeless
static TOOLSFIND  toolsfind;  // structure used for "find" modeless

// environment
static char       envPathName[MAX_PATH_LEN];  // environment file name
static char       envPassword[PASSWORD_LEN];  // environment password
static BOOL       bEnvPassword;               // environment with password?
BOOL  bSaveRecommended;                       // dirty environment?

// for critical sections
static int        criticalSectionCount;     // Reentrance pb solve
static int        criticalConnectCount;     // IIPROMPTALL solve
static HWND       criticalFocus;            // focus when entered critical
static HWND       criticalActive;           // active when entered critical

static BOOL       bOpenedMenu;              // menu currently opened

// Clipboard variables
static HWND hWndNextViewer;             // handle on next clipboard viewer
static char szDomClipboardFormat[] = "DbaToolsDomClipboardFmt";
WORD wDomClipboardFormat;
BOOL bDomClipboardDataStored;   // data stored with dom format
int  DomClipboardObjType;       // type of the last object copied in clipbd

// SPECIAL CASE : local variable whose address is passed as a parameter
// to another module : we need a far pointer for it!
static char FAR szFind[BUFSIZE];    // search string for find

//
// Forward declarations of local functions
//

static VOID  NEAR CloseOpenedCombobox(void);
static VOID  NEAR AppSize(HWND hwnd, UINT state, int cx, int cy);
BOOL   CleanupAndExit(HWND hwnd);
static LONG  NEAR CommandHandler (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify, DWORD dwData);
static VOID  NEAR CloseAllChildren(BOOL bKeepNodeList);
static BOOL  NEAR InitializeApplication(VOID);
static BOOL  NEAR InitializeInstance(LPSTR lpCmdLine, int nCmdShow);
static VOID  NEAR TerminateInstance(VOID);
static BOOL  NEAR CommandForMDIDoc(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static int   NEAR QueryCurDocType(HWND hwnd, HWND *pHwndDoc);
static VOID  NEAR MenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags);
static BOOL  NEAR LoadDocMenuString(int wID, HMENU hmenu, UINT flags, UCHAR *rsBuf, int DocType);
static BOOL  NEAR ChangeModeForBkRefresh(HWND hwnd);
static VOID  NEAR UpdateAppTitle(void);
static BOOL  NEAR OpenEnvironment(HWND hwnd, LPSTR lpszCmdLine);
static BOOL  NEAR SaveAsEnvironment(HWND hwnd);
static BOOL  NEAR SaveEnvironment(HWND hwnd, char *envName, char *passwordName, BOOL bPassword);
BOOL ProcessFindDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL  ManageDomPreferences(HWND hwndMain, HWND hwndParent);

// Probably temporary -Version marker checker
#define VERSIONSIZE     54  // Marker string stored length,
                            // including compress keyword size
#define COMPRESSKEYSIZE 16  // compress keyword size ( ":CAZIP:16" )

static int  NEAR DOMSaveVersionMarker(FILEIDENT idFile);
static int  NEAR DOMLoadVersionMarker(FILEIDENT idFile);

// environment security management
static int  NEAR DOMSavePassword(FILEIDENT idFile, char *passwordName, BOOL bEnvPassword);
static int  NEAR DOMLoadPassword(FILEIDENT idFile, BOOL *pbStopHere);

LRESULT CALLBACK CommonDefFrameProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
static void MainOnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest);
void        MainOnDestroy(HWND hwnd);
void        MainOnTimer(HWND hwnd, UINT id);
static void MainOnClose(HWND hwnd);
void        MainOnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags);
static void MainOnSysCommand(HWND hwnd, UINT cmd, int x, int y);
void        MainOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
BOOL        MainOnSpecialCommand(HWND hwnd, int id, DWORD dwData);
static void MainOnDrawClipboard(HWND hwnd);
static void MainOnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext);
void        MainOnSize(HWND hwnd, UINT state, int cx, int cy);
void        MainOnWinIniChange(HWND hwnd, LPCSTR lpszSectionName);
BOOL        MainOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static BOOL VerifyOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void VerifyOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
static void DomPrefOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

// Sql Statements errors display
// definition moved to DgErrH.h: DisplayStatementsErrors(HWND hWnd);
BOOL WINAPI __export SqlErrorsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
static BOOL SqlErrOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
static void SqlErrOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

static void SqlErrOnDestroy(HWND hDlg);

// refresh time-outs, in milliseconds

// Set to very long value (10 minutes) whatever MAINWIN is defined or not,
// since status does not feature time anymore.
// Rest of items updated thanks to OnIdle() MFC service
// Kept for dom trees resequencing function
#define GLOBALSTATUS_TIMEOUT  600000

//
// Settings
//

static BOOL bShowNanBar = FALSE;
static BOOL bShowStatus = FALSE;

static BOOL bShowRibbon = TRUE;

//
// RETURN TRUE only if User press the OK button and marks
// the check box as uncheck:
// NOTE: For convenience, this box is prepared for displaying 3 lines of text
//       of 80 70 characters at most.
//       If the text line in 'lpszMessage' is too long, you should break it by
//       adding the '\n':
extern BOOL MSGContinueBox (LPCTSTR lpszMessage);

//
// debugging mfc help
//
#ifdef DEBUG_CONTEXT_HELP
static BOOL VdbaWinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD dwData)
{
  if (uCommand == HELP_CONTEXT) {
    char buf[200];
    wsprintf(buf, "Context Help Id %lu requested (offsetmenu 20000)", dwData);
    MessageBox(hWndMain, buf, "Context Help Debugging", MB_ICONEXCLAMATION | MB_OK);
    return TRUE;
  }
  else
    return WinHelp(hWndMain, lpszHelp, uCommand, dwData);
}
#else   // DEBUG_CONTEXT_HELP
//#define VdbaWinHelp(a,b,c,d) (WinHelp(a,b,c,d))
static BOOL VdbaWinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD dwData)
{
	UINT nHelpID = 0;
	UINT nHelpIDDefault = 50001; //vdba_overview
	HH_FTS_QUERY q;
	memset (&q, 0, sizeof(q));
	q.cbStruct  = sizeof(q);
	switch (uCommand)
	{
	case HELP_CONTEXT:
		nHelpID = (UINT)dwData;
		HtmlHelp(hWndMain, lpszHelp, HH_HELP_CONTEXT, nHelpID);
		break;
	case HELP_FINDER:
		HtmlHelp(hWndMain, lpszHelp, HH_DISPLAY_SEARCH, (DWORD)&q);
		break;
	case HELP_PARTIALKEY:/*Help Contents*/
		HtmlHelp(hWndMain, lpszHelp, HH_DISPLAY_TOC, (DWORD)nHelpIDDefault);
		break;
	default:
		HtmlHelp(hWndMain, lpszHelp, HH_HELP_CONTEXT, (DWORD)nHelpIDDefault);
		break;
	}
	return TRUE;
}
#endif  // DEBUG_CONTEXT_HELP


HWND GetVdbaDocumentHandle(HWND hwndDoc)
{
  char className[40];
  HWND hwndTheDoc = GetWindow(hwndDoc, GW_CHILD);
  //ASSERT(hwndTheDoc);

  // check hwndDoc first
  className[0] = '\0';
  GetClassName(hwndDoc, className, sizeof(className)-1);
  if (!x_stricmp(className, szDomMDIChild))
    return hwndDoc;

  // loop on hwndDoc children, on 2 sub levels, in case splitbar in dom
  while(hwndTheDoc) {
    HWND hwndTheChildDoc = GetWindow(hwndTheDoc, GW_CHILD);
    className[0] = '\0';
    GetClassName(hwndTheDoc, className, sizeof(className)-1);
    if (!x_stricmp(className, szDomMDIChild))
      return hwndTheDoc;
    while(hwndTheChildDoc) {
      className[0] = '\0';
      GetClassName(hwndTheChildDoc, className, sizeof(className)-1);
      if (!x_stricmp(className, szDomMDIChild))
        return hwndTheChildDoc;
      hwndTheChildDoc = GetWindow(hwndTheChildDoc, GW_HWNDNEXT);
    }
    hwndTheDoc = GetWindow(hwndTheDoc, GW_HWNDNEXT);
  }
  //ASSERT (FALSE);
  return 0;   // Not Found!
}

// added 13/06/97 for mfc version
// Note : GetActiveWindow() compared to hwndFrame DOES NOT WORK when a dialog/message box
// is displayed! ---> need to flag the application active/inactive state!

static BOOL  bActiveApp           = FALSE;
static int   storedActiveDocType  = 0;
static HWND  storedFocusWindow    = 0;

// Added Dec. 18, 97: bring back application to top after a IIPROMPTALL if necessary
static BOOL bWasActive = FALSE;
static HWND hwndWasActiveWindow;

void MainOnActivateApp(BOOL fActive)
{
  if (fActive) {
    bActiveApp = TRUE;
  }
  else {
    // deactivated : save important data
    storedActiveDocType = QueryCurDocType(hwndFrame, NULL);   // Note: will be TYPEDOC_NONE if no doc
    storedFocusWindow   = GetFocus();

    // Emb 16/06/97: important check related to deactivation
    if (storedFocusWindow) {
      if (GetParent(storedFocusWindow) == hDlgRefresh)
        storedFocusWindow = NULL;
    }

    bActiveApp = FALSE;
  }
}

BOOL IsTheActiveApp()
{
  return bActiveApp;
}

HWND  GetStoredFocusWindow()
{
  return storedFocusWindow;
}

//
// Added Emb 25/6/97 to prevent disable of application
// if expanding comboboxes in toolbars calling domgetfirst()
//
static BOOL bExpandingCombobox = FALSE;
void SetExpandingCombobox()   { bExpandingCombobox = TRUE; }
void ResetExpandingCombobox() { bExpandingCombobox = FALSE; }
BOOL ExpandingCombobox() { return bExpandingCombobox; }

//
// Locking for reentrance problem solve when executing sql request
//
void StartSqlCriticalSection()
{
  #ifdef NOCRITICALSECTION
  return;
  #endif

  #ifdef DEBUGSQL
   return;
  #endif

  // pre increment BEFORE special management
  criticalSectionCount++;

  if (criticalSectionCount == 1) {    // just increased from 0 to 1
    // store "active app Y/N" information
    if (IsTheActiveApp()) {
      bWasActive = TRUE;
      hwndWasActiveWindow = GetActiveWindow();
    }
    else {
      bWasActive = FALSE;
      hwndWasActiveWindow = NULL;
    }
    #ifdef CRITICALONLYREACTIVATE
    return;
    #endif

    criticalFocus = GetFocus();
    if (DialogInProgress())
      //criticalActive = GetCurrentDialog();    // makes flashes
      //criticalActive = GetFocus();              // izzat enough?
      criticalActive = 0;
    else 
      criticalActive = hwndFrame;

#ifdef MAINWIN
    // 18/07/97 : need not to call EnableWindow() for ipm diskinfo under unix
    criticalActive = 0;
#endif

    // Emb 24-25/06/97: need to ignore if expanding comboboxes in toolbars calling domgetfirst()
    if (ExpandingCombobox())
      criticalActive = 0;

    if (criticalActive)
      EnableWindow(criticalActive, FALSE);  // Don't accept mouse/kbd messages
  }
}

void StopSqlCriticalSection()
{
  HWND activeWindow;

  #ifdef NOCRITICALSECTION
  return;
  #endif

  #ifdef DEBUGSQL
   return;
  #endif

  if (criticalSectionCount > 0)
    criticalSectionCount--;
  if (criticalSectionCount == 0) {
    if (criticalConnectCount)
      criticalConnectCount = 0;         // just in case...

    #ifdef CRITICALONLYREACTIVATE
	/* UKS BUG # 105480
    if (bWasActive) {
      if (GetForegroundWindow() != hwndWasActiveWindow)
        SetForegroundWindow(hwndWasActiveWindow);
    }
	*/
    return;
    #endif

    if (criticalActive) {
      EnableWindow(criticalActive, TRUE); // Accept mouse/kbd messages again

      // get active window handle for the current thread
      activeWindow = GetActiveWindow();

      // Emb 16/6/97 : Added test to set the active window only if the app. is active,
      // otherwise bar turns to blue as if vdba was the active application
      // NOTE: works even if another application has been brought to top while the
      // background refresh was taking place
      if (IsTheActiveApp()) {
        //if (activeWindow != hwndFrame)
        if (activeWindow != criticalActive)
          SetActiveWindow(criticalActive);  // bring app to top, for IIPROMPTALL
      }
    }
    // Emb 16/6/97 : Added test to set the focus only if the app. is active,
    // otherwise bar turns to blue as if vdba was the active application
    if (IsTheActiveApp()) {
      if (criticalFocus)
        SetFocus(criticalFocus);
      else
        SetFocus(hwndFrame);
    }

    // manage back to top after a IIPROMPTALL
    if (bWasActive) {
      if (GetForegroundWindow() != hwndWasActiveWindow)
        SetForegroundWindow(hwndWasActiveWindow);
    }

  }
}

BOOL InSqlCriticalSection()
{
  #ifdef NOCRITICALSECTION
  return FALSE;
  #endif

  if (criticalSectionCount)
    return TRUE;
  else
    return FALSE;
}

//
// Management for connect with IIPROMPTALL against reentrance
//

static BOOL   bMessageStored;
static HWND   hwndStored;
static UINT   wMsgStored;
static WPARAM wParamStored;
static LPARAM lParamStored;

void StoreMessage(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  bMessageStored  = TRUE;
  hwndStored      = hwnd;
  wMsgStored      = wMsg;
  wParamStored    = wParam;
  lParamStored    = lParam;
}

void StartCriticalConnectSection()
{
  #ifdef NOCRITICALSECTION
  return;
  #endif

  // pre increment BEFORE special management
  criticalConnectCount++;

  if (criticalConnectCount == 1) {    // just increased from 0 to 1
    bMessageStored = FALSE;
  }
}

void StopCriticalConnectSection()
{
  #ifdef NOCRITICALSECTION
  return;
  #endif

  if (criticalConnectCount > 0) {
    criticalConnectCount--;
    if (criticalConnectCount == 0) {
      if (bMessageStored)
        PostMessage(hwndStored, wMsgStored, wParamStored, lParamStored);
      bMessageStored = FALSE;
    }
  }
}

BOOL InCriticalConnectSection()
{
  #ifdef NOCRITICALSECTION
  return FALSE;
  #endif

  if (criticalConnectCount > 0)
    return TRUE;
  else
    return FALSE;
}

// new flag for dialog in progress, for INITDIALOG phase
static int  vdbaDialog;

void PushVDBADlg()
{
  vdbaDialog++;
}

void PopVDBADlg()
{
  vdbaDialog--;
}

int WINAPI VdbaDialogBox(HINSTANCE hInstance, LPCSTR lpCstr, HWND hWnd, DLGPROC dlgProc)
{
  int retval;
  vdbaDialog++;
  retval = DialogBox(hInstance, lpCstr, hWnd, dlgProc);
  vdbaDialog--;
  return retval;
}

int WINAPI VdbaDialogBoxParam(HINSTANCE hInstance, LPCSTR lpCstr, HWND hWnd, DLGPROC dlgProc, LPARAM lParam)
{
  int retval;
  vdbaDialog++;
  retval = DialogBoxParam(hInstance, lpCstr, hWnd, dlgProc, lParam);
  vdbaDialog--;
  return retval;
}

//
//  Is there a dialog box in progress ?
//
// modified Emb 13/06 for mfc version
// since we have a lot of modeless dialogs
// (nodes selection, dbevents, etc...)
//
BOOL DialogInProgress()
{
  HWND  hwnd;
  char  bufClassName[10];
  int   activeDocType;
  HWND  focusWindow;

  // "flagged" dialog box or message box, whatever mfc or not
  if (vdbaDialog > 0)
    return TRUE;

  // search modeless dialog: act as if modal
  if (hDlgSearch && IsWindow(hDlgSearch))
    return TRUE;

  // variables for next step
  if (bActiveApp) {     // cannot compare GetActiveWindow() to hwndFrame, see previous comment
    // appli is foreground
    activeDocType = QueryCurDocType(hwndFrame, NULL);   // Note: will be TYPEDOC_NONE if no doc
    focusWindow   = GetFocus();
  }
  else {
    // appli is backgrounded
    activeDocType = storedActiveDocType;
    focusWindow   = storedFocusWindow;
  }

  // act as if no dialog box if "new type mdi doc"
  switch (activeDocType) {
    case TYPEDOC_MONITOR:
    case TYPEDOC_DBEVENT:
    case TYPEDOC_NODESEL:
    case TYPEDOC_NONE:
      return FALSE;
  }

  // old type mdi doc, or no document at all: use old criterion
  hwnd = focusWindow;
  while (hwnd) {
    GetClassName(hwnd, bufClassName, sizeof(bufClassName));
    if (x_strcmp(bufClassName, "#32770") == 0) {
      if (hwnd != GetMfcNodesToolbarDialogHandle())
        return TRUE;
      else
        return FALSE;   // Global nodes toolbar dlg ---> I don't want it!
    }
    hwnd = GetParent(hwnd);
  }
  return FALSE;
}

//
//  return handle of dialog box in progress, or NULL if no dialog
//
HWND GetCurrentDialog()
{
  HWND  hwnd;
  char  bufClassName[50];

  hwnd = GetFocus();
  while (hwnd) {
    GetClassName(hwnd, bufClassName, sizeof(bufClassName));
    if (x_strcmp(bufClassName, "#32770") == 0)
      return hwnd;
    hwnd = GetParent(hwnd);
  }
  return NULL;
}


static BOOL bShowBkRefreshWarning = TRUE;
//
//  Manages the background refresh on/off
//
static BOOL  NEAR ChangeModeForBkRefresh(HWND hwnd)
{
    if (bShowBkRefreshWarning && OneWinExactlyGetWinType()!=TYPEDOC_NONE) {
         char szTitle[BUFSIZE];
        VDBA_GetProductCaption(szTitle, sizeof(szTitle));

       if (MSGContinueBox (ResourceString(IDS_WARNING_BKRF_GLOBAL_SETTING)))
           bShowBkRefreshWarning = FALSE;
    }
   bBkRefreshOn = !bBkRefreshOn;
  // store new state since it was a user action on the menu
  SaveBkTaskEnableState();

  return TRUE;
}

//
//  manages the background task - called periodically
//
static VOID NEAR ManageBkTask(void)
{
  BOOL   bMustRefreshDbevents    = TRUE;
  BOOL   bMustRefreshBackground  = TRUE;

  if (CanBeInSecondaryThead()) // animation in progress, refuse reentrance
	  return;
  // Check not in SQL critical section
  if (InSqlCriticalSection())
    return;

  if (IsIconic(hwndFrame)) {
     // application in an iconic state
     bMustRefreshBackground = FALSE;
  }

  if (DialogInProgress()) {
     // current window is a dialog box
     // (includes print or preferences in progress)
     bMustRefreshDbevents    = FALSE;
     bMustRefreshBackground  = FALSE;
  }

  if (!bActiveApp) {     // cannot compare GetActiveWindow() to hwndFrame, see previous comment
     // the app is not the active application
     // Nothing to do
  }

  // verdict
  if (bMustRefreshBackground || bMustRefreshDbevents) {

    // deactivate
    StopBkTask();

    // refresh what needs to be
    if (bMustRefreshBackground) {
      // if bk refresh enabled, background task refresh procedure
      if (bBkRefreshOn && !IsInvokedInContextWithOneWinExactly())
        DOMBkRefresh(ESL_time(),FALSE,NULL);
    }
    if (bMustRefreshDbevents) {
     // Call DbEvents scan
     DBEScanDBEvents();
    }

    // reactivate
    ActivateBkTask();
  }
}

//
// Says whether the mouse is on the global toolbar or not
//
static BOOL IsCursorOnGlobalToolbar()
{
  RECT  rc;
  POINT pt;

  // Say no if no toolbar
  if (!ToolBarPref.bVisible)
    return FALSE;

  GetWindowRect(hwndToolBar, &rc);
  GetCursorPos(&pt);
  if (PtInRect(&rc, pt))
    return TRUE;
  else
    return FALSE;
}

//
//  manages the global status bar switch/Update - called periodically
//
static VOID NEAR ManageGlobalStatus(void)
{
  // nothing to do if a menu is opened
  if (bOpenedMenu)
    return;

  // check bktask with info on global bar not in progress
  if (ESL_IsRefreshStatusBarOpened ())
    return;

  // Here, either we don't have a global toolbar, or the mouse is not on it
  // We can update the global status bar
  bGlobalStatus = TRUE;
  UpdateGlobalStatus();
}

//
//  manages the doms status bars - called periodically
//
static VOID NEAR UpdateDomStatus(void)
{
  // Splitbar version: Nothing to do, managed by the dom itself
}

//
//  manages the doms trees resequencing - called periodically
//
static VOID NEAR ResequenceDomTrees(void)
{
  HWND      hwndCurDoc;
  int       docType;
  LPDOMDATA lpDomData;

  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  while (hwndCurDoc) {
    docType = QueryDocType(hwndCurDoc);
    if (docType==TYPEDOC_DOM) {
      lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
      ResequenceTree(GetVdbaDocumentHandle(hwndCurDoc), lpDomData);
    }

    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }
}

// Winmain masqued if we are generating a static library
// for use with a C++ skeleton, replaced by 2 functions

int WinMainEnter(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
  char buf[BUFSIZE];

  hInst = hInstance;    // global variable

#ifdef RC_EXTERNAL
  if ((hResource=LoadLibrary("VDBA.LNG"))==NULL)
#endif
  	hResource=hInstance;

#ifdef _USE_CTL3D
  // CRASHES WHEN PORTING TO UNIX!
  // ctl3d use
  Ctl3dRegister(hInst);
  Ctl3dAutoSubclass(hInst);
#endif

  // Initialize icons and bitmaps caches - Must be done before InitializeApp.
  IconCacheInit();
  BitmapCacheInit();

  // Initialize ingres version
  SetOIVers(OIVERS_30);


  // Load multiple menus added July 26, 96
  hGwNoneMenu = LoadMenu(hResource, MAKEINTRESOURCE(ID_MENU_GWNONE));

  gMemErrFlag = MEMERR_NOMESSAGE;

  // If this is the first instance of the app. register window classes
  if (!hPrevInstance)
    if (!InitializeApplication ()) {
      TerminateInstance();  // free objects previously allocated (caches)
      return 0;
    }

  // Create the frame and do other initialization
  if (!InitializeInstance (lpszCmdLine, nCmdShow)) {
    TerminateInstance();  // free objects previously allocated (caches,fonts)
    return 0;
  }

  if (gMemErrFlag == MEMERR_HAPPENED) {
    LoadString(hResource, IDS_E_MEMERR_STARTUP, buf, sizeof(buf));
    MessageBox(NULL, buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return FALSE;
  }
  gMemErrFlag = MEMERR_STANDARD;

  //
  // Vut Adds 30-Aug-1995
  // Begin
  //
  lpHookProcInstance = MakeProcInstance (HookFilterFunc, hInst);
  if (lpHookProcInstance == NULL)
     return FALSE;

  //hHookFilter = SetWindowsHookEx (WH_MSGFILTER, lpHookProcInstance);
  // taken from sample
  hHookFilter = SetWindowsHookEx (WH_MSGFILTER, lpHookProcInstance,
                                  (HINSTANCE) NULL, GetCurrentThreadId());
  //
  // Vut End
  // 

  //
  // Vut Adds, Load string from resource. 
  //  
  gMemErrFlag = MEMERR_NOMESSAGE;
  PrivilegeString_INIT ();

  if (gMemErrFlag == MEMERR_HAPPENED) {
    LoadString(hResource, IDS_E_MEMERR_STARTUP, buf, sizeof(buf));
    MessageBox(NULL, buf, NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    return FALSE;
  }
  gMemErrFlag = MEMERR_STANDARD;

  //
  // Vut Ends
  //

  // SQL Errors management
  EnableSqlErrorManagmt();
  // Assign temporary file name for sql errors
  if ( !VdbaInitializeTemporaryFileName()) {
    MessageBox(NULL, ResourceString ((UINT) IDS_E_CREATE_TEMPORARY_FILE), NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
    DisableSqlErrorManagmt();
  }

  return 1;
}


int WinMainTerminate()
{
  HCURSOR hOldCursor;

  // SQL Errors management
  DisableSqlErrorManagmt();

  // No more memory message until the end of the product
  gMemErrFlag = MEMERR_NOMESSAGE;

  // ctl3d use
#ifdef _USE_CTL3D
  Ctl3dUnregister(hInst);
#endif

  // cleanup connexions and other (caches,...)
  hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
  TerminateInstance();
  SetCursor(hOldCursor);
  FreeSqlErrorPtr();
  ESL_RemoveFile(VdbaGetTemporaryFileName());

  //
  // Vut Adds, Destruction and clean up 
  //
  UnhookWindowsHookEx(hHookFilter);
  FreeProcInstance   (lpHookProcInstance);
  if (lpHelpStack)
    lpHelpStack = StackObject_FREE (lpHelpStack);
  PrivilegeString_DONE ();

  //
  // Vut ends 

  // check remaining memory
  #ifdef DEBUGMALLOC
  {
    char buf[BUFSIZE];

    wsprintf(buf, "remaining allocated memory chunks : %d",
         ESL_AllocCount());
    MessageBox(GetFocus(), buf, NULL,
               MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
  }
  #endif //DEBUGMALLOC

#ifdef NEWFE
endwm:
#endif

  return 0;
}


//
// Vut adds 30-Aug-1995
//

LRESULT CALLBACK HookFilterFunc (int nCode, WPARAM wParam, LPARAM lParam)
{
  static BOOL bCtrl;
  LPMSG lpmsg = (LPMSG)lParam;

  if ((nCode == MSGF_DIALOGBOX || nCode == MSGF_MENU) && lpmsg->message == WM_KEYDOWN && lpmsg->wParam == VK_F1)
  {
      PostMessage (hwndFrame, WM_F1DOWN, nCode, 0L);
  }
  //DefHookProc (nCode, wParam, lParam, &hHookFilter);

  // Added Emb 3/11 to manage Ctrl+E in dialog boxes
  // Fixed Emb + Ps for key combination AltGr+E mistaken with Ctrl+E
  if (nCode==MSGF_DIALOGBOX) {
    if (lpmsg->message == WM_KEYDOWN) {
      if (lpmsg->wParam == VK_CONTROL)	// Note : AltGr also generates VK_CONTROL!!!
        bCtrl = TRUE;
      if (lpmsg->wParam == 'E') {     // VK_E not defined, see windows.h
        if (bCtrl) {
          // Test bit 29 of lParam to distinguish between Ctrl and AltGr
          if (! (lpmsg->lParam & 0x20000000) ) {
            DisplayStatementsErrors(GetCurrentDialog());
            bCtrl = FALSE;
            return 1;   // Discard the 'E'
          }
        }
      }
    }
  
    // reset "Ctrl" flag when release of Ctrl and AltGr keys
    if (lpmsg->message == WM_KEYUP) {
      if (bCtrl) {
        // release of Ctrl
        if (lpmsg->wParam == VK_CONTROL)
          bCtrl = FALSE;
        // release of AltGr
        if (lpmsg->wParam == VK_MENU)
          bCtrl = FALSE;
      }
    }

  } // end of if (nCode==MSGF_DIALOGBOX)

  return CallNextHookEx (hHookFilter, nCode, wParam, lParam);
}

void VDBA_TRACE0 (LPCTSTR lpszText);
void MainOnF1Down (int nCode)
{
#if defined (_DEBUG) && defined (WIN32)
   TCHAR tchszText [128];
   wsprintf (tchszText, "MainOnF1Down (nCode = %d)\n", nCode);
   VDBA_TRACE0 (tchszText);
#endif

   switch (nCode)
   {
       case MSGF_DIALOGBOX:
       {
           #if defined (_DEBUG) && defined (WIN32)
           wsprintf (tchszText, "Help Context Id of Dialog Box = %d\n", lpHelpStack? lpHelpStack->Id: -1);
           VDBA_TRACE0 (tchszText);
           #endif
           if (lpHelpStack && (lpHelpStack->Id != OFFSETHELPMENU) )
               VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, (DWORD)lpHelpStack->Id);
           else
             if (lpHelpStack)
               VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_PARTIALKEY, (DWORD)"");   // for .cnt file
             else
               // no help stack ---> certainly property page active
               // ---> let idm_contexthelp do the job
               CommandHandler (hwndFrame, IDM_CONTEXTHELP, 0, 0, 0);
           break;
       }

       case MSGF_MENU:
       {
          if (dwMenuHelpId &&
            ( dwMenuHelpId < IDM_WINDOWFIRSTCHILD+OFFSETHELPMENU ||
              dwMenuHelpId > IDM_WINDOWFIRSTCHILD+OFFSETHELPMENU+50) )
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, dwMenuHelpId);
          else 
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_PARTIALKEY, (DWORD)"");   // for .cnt file

#if defined (_DEBUG) && defined (WIN32)
          // NOTE : MESSAGE BOX AFTER HELP CALL SINCE RESETS DWMENUHELPID
          // BECAUSE MENU IS CLOSED!
            {
                TCHAR tchszText [256];
                if (dwMenuHelpId && (dwMenuHelpId < IDM_WINDOWFIRSTCHILD+OFFSETHELPMENU ||dwMenuHelpId > IDM_WINDOWFIRSTCHILD+OFFSETHELPMENU+50))
                    wsprintf (tchszText, "Help for menu item %ld was requested\n", dwMenuHelpId);
                else 
                    wsprintf (tchszText, "Context Help uneligible menu item %ld\n", dwMenuHelpId);
                VDBA_TRACE0 (tchszText);
            }
#endif   // DEBUGHELP

          break;
       }
   }
}



//
// Vut Ends
//


//
// FrameWndProc(hwnd, wMsg, wParam, lParam )
//
// Window function for the Frame window
//
LRESULT CALLBACK __export FrameWndProc ( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
  char                rsBuf[BUFSIZE];

  // Check for Find dialog notification messages.
  if (ProcessFindDlg(hwnd, wMsg, wParam, lParam))
    return 0L;

    if (wMsg == uNanNotifyMsg)
    {
    switch (wParam)
    {
        case NANBAR_NOTIFY_MSG:
        {
        int nStringId = (int)LOWORD(lParam);

        // the icon bar notifies us for displaying a text in the status bar
        if (!ESL_IsRefreshStatusBarOpened ())
        {
            if (StatusBarPref.bVisible) 
            {
            rsBuf[0] = '\0';
            LoadString(hResource, nStringId, rsBuf, sizeof(rsBuf));
            Status_SetMsg(hwndStatusBar, rsBuf);
            // Whatever the string is empty or not
            bGlobalStatus = FALSE;
            if (StatusBarPref.bVisible)
                BringWindowToTop(hwndStatusBar);
            }
        }

        break;
        }

        // Not to be used since we have our cache functions in winutils
        //case NANBAR_NOTIFY_FREEBMP:
        //  {
        //    HBITMAP hBitmap = (HBITMAP)lParam;
        //    DestroyBitmap(hBitmap);
        //  }
        //  break;
    }

    return 0;
    }

  switch (wMsg) {
    HANDLE_MSG(hwnd, WM_NCLBUTTONDOWN, MainOnNCLButtonDown);
    HANDLE_MSG(hwnd, WM_DESTROY, MainOnDestroy);
    HANDLE_MSG(hwnd, WM_TIMER, MainOnTimer);
    HANDLE_MSG(hwnd, WM_CLOSE, MainOnClose);
    HANDLE_MSG(hwnd, WM_MENUSELECT, MainOnMenuSelect);
    HANDLE_MSG(hwnd, WM_SYSCOMMAND, MainOnSysCommand);
    HANDLE_MSG(hwnd, WM_COMMAND, MainOnCommand);
    HANDLE_MSG(hwnd, WM_DRAWCLIPBOARD, MainOnDrawClipboard);
    HANDLE_MSG(hwnd, WM_CHANGECBCHAIN, MainOnChangeCBChain);
    HANDLE_MSG(hwnd, WM_SIZE, MainOnSize);
    HANDLE_MSG(hwnd, WM_WININICHANGE, MainOnWinIniChange);
    HANDLE_MSG(hwnd, WM_CREATE, MainOnCreate);

    // TO BE FINISHED : NEEDS MESSAGE CRACKER FOR WINDOWS NT
    //
    // Vut Adds 30-Aug-1995
    //

    case WM_F1DOWN:
       MainOnF1Down ((int)wParam);
       break;

    //
    // Vut Ends
    //
    default:
      return CommonDefFrameProc (hwnd,wMsg,wParam,lParam);
  }
  return 0L;
}

LRESULT CALLBACK CommonDefFrameProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return DefFrameProc(hwnd, hwndMDIClient, message, wParam, lParam);
}



void ManageSessionPreferenceSettup(HWND hWndDialog)
{
    int  iRet;
    SESSIONPARAMS sess;
    HWND OldFocus = GetFocus ();

    memcpy(&sess,&GlobalSessionParams,sizeof(SESSIONPARAMS));
    iRet = DlgSession (hWndDialog, &sess);
    SetFocus (OldFocus);
    if (iRet == IDOK)
    {
        if (IsSavePropertyAsDefault())
            WriteSessionSettings (&sess);
        memcpy(&GlobalSessionParams,&sess,sizeof(SESSIONPARAMS));
        imaxsessions=(int)GlobalSessionParams.number;
    }
}


static void MainOnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
{
    CloseOpenedCombobox();
    FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, CommonDefFrameProc);
}

void MainOnDestroy(HWND hwnd)
{
  // clear our custom DOM format the clipboard,
  // because of Attached Pointers free management
  ClearDomObjectFromClipboard(hwnd);
  ChangeClipboardChain(hwnd, hWndNextViewer);

  // stop timers
  KillTimer(hwnd, IDT_GLOBALSTATUS);

  PostQuitMessage (0);
}

void MainOnTimer(HWND hwnd, UINT id)
{
  switch (id) {
    case IDT_GLOBALSTATUS:
      ManageGlobalStatus();
      ResequenceDomTrees();
      break;

    case IDT_BKTASK:
      ManageBkTask();
      break;

    default:
      assert(FALSE);    // Unexpected!
      break;
  }
}

static void MainOnClose(HWND hwnd)
{
    CleanupAndExit(hwnd);
}

void MainOnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
    MenuSelect(hwnd, hmenu, item, hmenuPopup, flags);
}

static void MainOnSysCommand(HWND hwnd, UINT cmd, int x, int y)
{
    //if (wParam == SC_CLOSE)
    //  CleanupAndExit(hwnd);
    FORWARD_WM_SYSCOMMAND(hwnd, cmd, x, y, CommonDefFrameProc);
}

// Custom function for special commands called from mfc version,
// using data
BOOL MainOnSpecialCommand(HWND hwnd, int id, DWORD dwData)
{
  return (BOOL)CommandHandler(hwnd, id, 0, 0, dwData);
}

void MainOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    CommandHandler (hwnd, id, hwndCtl, codeNotify, 0);
}

static void MainOnDrawClipboard(HWND hwnd)
{
  // ???...
  if (hWndNextViewer)
    FORWARD_WM_DRAWCLIPBOARD(hWndNextViewer, SendMessage);
}

static void MainOnChangeCBChain(HWND hwnd, HWND hwndRemove, HWND hwndNext)
{
  // Change in the clipboard chain
  if (hwndRemove == hWndNextViewer)
    hWndNextViewer = hwndNext;
  else
    if (hWndNextViewer)
      FORWARD_WM_CHANGECBCHAIN(hWndNextViewer, hwndRemove, hwndNext, SendMessage);
}

void MainOnSize(HWND hwnd, UINT state, int cx, int cy)
{
    AppSize(hwnd, state, cx, cy);
}

static void MainOnWinIniChange(HWND hwnd, LPCSTR lpszSectionName)
{
    //LoadWininiDateTimeFmt();
    UpdateGlobalStatus();
}

BOOL MainOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
  char                rsBuf[BUFSIZE];
  HMENU hMenu;

  initNodeStruct();
  MdiSelWinInit(hwndMDIClient);

  hMenu = GetMenu(hwnd);

  // Read preferences from the preference file
  if (!DBAGetDefaultSettings(hwnd)) {
     // TO BE FINISHED: Put A Message;
    return FALSE;
  }

  // Say toolbar invisible
  ToolBarPref.bVisible = FALSE;

  // Say toolbar invisible
  StatusBarPref.bVisible = FALSE;

  // Initialize CHMODREFRESH menuitem state
  if (bBkRefreshOn) {
    CheckMenuItem(hMenu,       IDM_CHMODREFRESH, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hGwNoneMenu, IDM_CHMODREFRESH, MF_BYCOMMAND | MF_CHECKED);
  }
  else {
    CheckMenuItem(hMenu,       IDM_CHMODREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hGwNoneMenu, IDM_CHMODREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
  }

  // Initialize the clipboard
  hWndNextViewer = SetClipboardViewer(hwnd);
  wDomClipboardFormat = RegisterClipboardFormat(szDomClipboardFormat);

  // acquire the win.ini informations about date and time format
  //LoadWininiDateTimeFmt();

  // Launch the timer for periodic global status bar update
  if (!SetTimer(hwnd, IDT_GLOBALSTATUS, GLOBALSTATUS_TIMEOUT, 0)) {
    LoadString(hResource, IDS_GS_ERR_TIMER, rsBuf, sizeof(rsBuf));
    MessageBox(hwnd, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
    return FALSE;    // Will stop create process
  }

  return TRUE;
}

//
// close the potentially opened combobox in mdi doc
//
static VOID NEAR CloseOpenedCombobox()
{
  LPDOMDATA     lpDomData;
  // not necessary anymore, see UK.S comment below: LPSQLACTDATA  lpSqlActData;
  HWND          curDocHwnd;
  int           curDocType;

  curDocType = QueryCurDocType(hwndFrame, &curDocHwnd);
  switch (curDocType) {
    case TYPEDOC_DOM:
      // nothing to code for desktop
      lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(curDocHwnd), 0);
      if (GetFocus() == lpDomData->hwndBaseOwner ||
      GetFocus() == lpDomData->hwndOtherOwner)
    SetFocus(lpDomData->hwndTreeLb);
      break;
  }
}

//
// Manage a change in size of the application
//
static VOID NEAR AppSize(HWND hwnd, UINT state, int cx, int cy)
{
  return;  // nothing to do for mainlib
}


//
//  Self-speaking name
//
//  Received parameter = handle of the frame window
//
BOOL CleanupAndExit(HWND hwnd)
{
  char  rsBuf[BUFSIZE];
  char  rsCapt[BUFSIZE];
  char buf2[BUFSIZE];
  int   iret;

  // Ask for user confirmation
  //#ifdef NOLOADSAVE
  //Turn off the confirmation for exit.
  bSaveRecommended = FALSE;
  //#endif
  if (bSaveRecommended) {
    LoadString(hResource, IDS_CONFIRM_EXIT, rsBuf, sizeof(rsBuf));
    LoadString(hResource, IDS_CONFIRM_EXIT_CAPT, rsCapt, sizeof(rsCapt));
	wsprintf(buf2,rsCapt,VDBA_GetInstallName4Messages());
    iret = MessageBox(hwnd, rsBuf, buf2,
                      MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL);
    switch (iret) {
      case IDYES:
    if (envPathName[0]) {
      if (!SaveEnvironment(hwnd, (LPSTR)NULL, (LPSTR)NULL, FALSE))
        return FALSE;   // don't exit the app
    }
    else {
      if (!SaveAsEnvironment(hwnd))
        return FALSE;   // don't exit the app
    }
    break;

      case IDNO:
    break;

      case IDCANCEL:
    return FALSE;       // don't exit the app
    }
  }

  // Loop to clean the opened MDI documents
  CloseAllChildren (FALSE);

  // Cleanup for the "Select Window" box
  MdiSelWinEnd(hwndMDIClient);
  return TRUE;
}


//
// CommandHandler (hwnd, id, hwndCtl, codeNotify, dwData)
//
// Manages all frame WM_COMMAND messages - dwData added for mfc
//
static LONG NEAR CommandHandler (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify, DWORD dwData)
{
  BOOL  bForMDIDoc;
  char  buf[BUFSIZE];
  char  bufEnv[BUFSIZE];
  char  rsBuf[BUFSIZE];
  char  rsCapt[BUFSIZE];
  char  buf2[BUFSIZE];
  int   mbRet;
  BOOL  bNoNew;
  HMENU hMenu;
  UINT  uiState;
  HWND  curDocHwnd;
  int   curDocType;

  // hook on windows menu item "More Windows ..."
  if (id == IDM_WINDOWFIRSTCHILD + 9)
    id = IDM_SELECTWINDOW;

  // If we received an accelerator,
  // check the corresponding menuitem is not grayed
  if (codeNotify == 1) {
    hMenu   = GetMenu(hwndFrame);
    uiState = GetMenuState(hMenu, id, MF_BYCOMMAND);
    if (uiState != -1) {  // -1: no menuitem for this wm_command
      if (uiState & (MF_GRAYED | MF_DISABLED))
    return FALSE;   // menuitem grayed or disabled: don't accept
    }
  }

  switch (id) {

    case IDM_ABOUT:
      myerror(ERR_OLDCODE);
      break;

    //
    // Help management Added Vut 29/08/95
    //

    case IDM_CONTEXTHELP:
      if ( GetVdbaHelpStatus () == FALSE)
        break; // display the help.file of IIA only
      // first, check whether the search dialog is active
      if (hDlgSearch && IsWindow(hDlgSearch)) { // masqued && DialogInProgress() 13/06/97 since function modified
        VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, IDM_FIND);  // 2011
        break;
      }
      // Manage help according to current mdi document type,
      // or special management if no document
      curDocType = QueryCurDocType(hwndFrame, &curDocHwnd);
      if (curDocHwnd) {
        switch (curDocType) {
          case TYPEDOC_DOM:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, GetDomDocHelpId(curDocHwnd));
            break;
          case TYPEDOC_SQLACT:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, 8003);
            break;

          // new cases added may 3, 97
          case TYPEDOC_MONITOR:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, GetMonitorDocHelpId(curDocHwnd));
            break;
          case TYPEDOC_DBEVENT:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, GetDbeventDocHelpId(curDocHwnd));
            break;
          case TYPEDOC_NODESEL:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, GetNodeselDocHelpId(curDocHwnd));
            break;
        
          default:
            VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_PARTIALKEY, (DWORD)"");   // for .cnt file
            break;
        }
      }
      else {
        // or special management if no document
        if (IsNodesWindowVisible())
          VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, HELPID_NODESEL);
        else
          VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_CONTEXT, HELPID_BLANKSCREEN);
      }
      break;

    case IDM_HELPINDEX:
      VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_PARTIALKEY, (DWORD)"");   // for .cnt file
      break;

    case IDM_HELPSEARCH:
      VdbaWinHelp (hwndFrame, VDBA_GetHelpFileName(), HELP_FINDER, (DWORD)0L);   // for .cnt file
      break;

    case IDM_NEW:
      // Request confirmation from the user if might be useful
      bNoNew = FALSE;     // default

#ifdef NOLOADSAVE
      bSaveRecommended = FALSE;
#endif
      if (bSaveRecommended) {
    LoadString(hResource, IDS_NEW_QUERYSAVE_TXT, rsBuf, sizeof(rsBuf));
    if (envPathName[0])
      x_strcpy(bufEnv, envPathName);
    else
      LoadString(hResource, IDS_NEW_UNTITLED_ENV, bufEnv, sizeof(bufEnv));
    wsprintf(buf, rsBuf, bufEnv);
    LoadString(hResource, IDS_NEW_QUERYSAVE_CAPT, rsCapt, sizeof(rsCapt));
	wsprintf(buf2,rsCapt,VDBA_GetInstallName4Messages());
    mbRet = MessageBox(hwndFrame, buf, buf2,
              MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL);
    switch (mbRet) {
      case IDCANCEL:
        bNoNew = TRUE;
        break;
      case IDNO:
        bNoNew = FALSE;
        break;
      case IDYES:
        if (envPathName[0]) {
          SaveEnvironment(hwnd, (LPSTR)NULL, (LPSTR)NULL, FALSE);
          bNoNew = FALSE;
        }
        else {
          if (SaveAsEnvironment(hwnd))
        bNoNew = FALSE;
          else
        bNoNew = TRUE;
        }
    }
      }

      // exit if new does not apply
      if (bNoNew)
    break;

      // Setup fresh environment
      CloseAllChildren(TRUE);   // don't close nodes window in mdi mode!

      // terminate / reinitialize cache
      EndNodeStruct();
      initNodeStruct();

      // minimal nodes tree
      MfcResetNodesTree();

      // reset sequential numbers for mdi docs captions
      seqDomNormal      = 0;    // normal dom sequential number
      seqDomTearOut     = 0;    // teared out dom sequential number
      seqDomRepos       = 0;    // repositioned dom sequential number
      seqDomScratchpad  = 0;    // scratchpad dom sequential number

      // reset environment
      envPathName[0] = '\0';
      envPassword[0] = '\0';
      bEnvPassword   = FALSE;
      bSaveRecommended = FALSE;

      UpdateAppTitle();
      break;

    case IDM_OPEN:
#ifdef NOLOADSAVE
      assert (FALSE);
#else
      MdiSelWinEnd(hwndMDIClient);
      if (OpenEnvironment(hwnd, NULL)) {
        SetOpenEnvSuccessFlag();
        bSaveRecommended = FALSE;
        UpdateAppTitle();
      }
      MdiSelWinInit(hwndMDIClient);
#endif
      break;

    case IDM_SAVE:
#ifdef NOLOADSAVE
      assert (FALSE);
#else
      if (SaveEnvironment(hwnd, (LPSTR)NULL, (LPSTR)NULL, FALSE)) {
    bSaveRecommended = FALSE;
    UpdateAppTitle();
      }
#endif
      break;

    case IDM_SAVEAS:
#ifdef NOLOADSAVE
      assert (FALSE);
#else
      if (SaveAsEnvironment(hwnd)) {
    bSaveRecommended = FALSE;
    UpdateAppTitle();
      }
#endif
      break;

    case IDM_FIND:
      {
    BOOL  bRet;

    //lstrcpy(szFind, "on solid ground");
    memset(&toolsfind, 0, sizeof(toolsfind));
    toolsfind.lpszFindWhat  = szFind;
    toolsfind.wFindWhatLen  = sizeof(szFind);
    bRet = DlgFind(hwnd, &toolsfind);
    if (bRet) {
      hDlgSearch = toolsfind.hdlg;    // store for accel translation
      EnableMenuItem(GetMenu(hwndFrame), IDM_FIND,
                         MF_BYCOMMAND | MF_GRAYED);
      NanBar_Disable(hwndToolBar, IDM_FIND);    // Added Emb Jan 15, 97
    }
    return 0L;
      }
      break;

    case IDM_CONNECT:
      // Make a DOM type MDI child window
      if (MakeNewDomMDIChild (DOM_CREATE_QUERYNODE, 0, 0, dwData))
        return 1L;    // will serve as true
      else
        return 0L;    // will serve as false
      break;

    case IDM_SCRATCHPAD:
      // Make a scratchpad on the given node whose name is in dwData
      if (MakeNewDomMDIChild (DOM_CREATE_SCRATCHPAD, 0, 0, dwData))
        return 1L;    // will serve as true
      else
        return 0L;    // will serve as false
      break;

    case IDM_DISCONNECT:
      {
       SERVERDIS dis;
       int res;

       _fmemset (&dis, 0, sizeof (dis));
       res =  DlgServerDisconnect (hwnd, &dis);
       if (res != IDOK)
           return 0L;
      }
      break;

    case IDM_FILEEXIT:
      // Close application
      SendMessage (hwnd, WM_CLOSE, 0, 0L);
      break;

    case IDM_PREFERENCES:
      // Display the configuration window

      PushVDBADlg();
      VDBA_DlgPreferences();
      PopVDBADlg();
      break;

    case IDM_CHMODREFRESH:
      ChangeModeForBkRefresh(hwnd);
      break;

    case IDM_SPACECALC:
      // Spawn the Space calculation box in dll dbadlg1
      DlgSpaceCalc(hwnd);
      break;

    case IDM_WINDOWCASCADE:
      SendMessage (hwndMDIClient, WM_MDICASCADE, 0, 0L);
      break;

    case IDM_WINDOWTILE_HORZ:
      SendMessage (hwndMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0L);
      break;

    case IDM_WINDOWTILE_VERT:
      SendMessage (hwndMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0L);
      break;

    case IDM_WINDOWICONS:
      SendMessage (hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
      break;

    case IDM_WINDOWCLOSEALL:
      CloseAllChildren(FALSE);
      break;

    case IDM_SELECTWINDOW:
      lpHelpStack = StackObject_PUSH (lpHelpStack,
                          StackObject_INIT ((UINT)IDD_SELECTWINDOW));
      MdiSelWinBox(hwndMDIClient);
      lpHelpStack = StackObject_POP (lpHelpStack);
      return 1L;
      break;

    case IDM_SQLERRORS:
      // Display the dialog box for sql statements errors history
      DisplayStatementsErrors(hwndMDIClient);
      break;

    default:
      // pass the message to the current mdi doc, if any
      bForMDIDoc = CommandForMDIDoc(hwnd, id, hwndCtl, codeNotify);

      // pass to def proc if not managed by the current mdi doc
      if (!bForMDIDoc)
    FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, CommonDefFrameProc);
    }
  return 0L;
}

//
// BOOL CommandForMDIDoc(hwnd, id, hwndCtl, codeNotify)
//
// Passes the message to the current MDI document.
// the MDI doc Command handler MUST return TRUE if message processed,
// FALSE if not processed
//
// This function MUST be updated for every new MDI document Class
//
static BOOL NEAR CommandForMDIDoc(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  BOOL bRet = FALSE;
  int  DocType;
  HWND hwndDoc;

  // identify the current document type
  DocType = QueryCurDocType(hwnd, &hwndDoc);
  switch (DocType) {
    case TYPEDOC_UNKNOWN:
      return FALSE;
    case TYPEDOC_NONE:
      return FALSE;
    case TYPEDOC_DOM:
      bRet = (BOOL) SendMessage(GetVdbaDocumentHandle(hwndDoc), WM_COMMAND, MAKEWPARAM(id, codeNotify), (LPARAM)(HWND)hwndCtl);
      return bRet;
  }
  return FALSE;
}

//
// int QueryCurDocType(HWND hwndFrame, HWND *pHwndDoc)
//
// returns the type of the current, or TYPEDOC_NONE if no document
//
// At this time, hwndFrame not used
//
static int NEAR QueryCurDocType(HWND hwndFrame, HWND *pHwndDoc)
{
  HWND hwndDoc;

  // dummy instructions to skip compiler warnings at level four
  hwndFrame;

  if (pHwndDoc)
    *pHwndDoc = 0;
  hwndDoc = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
  if (hwndDoc==0)
    return TYPEDOC_NONE;
  if (pHwndDoc)
    *pHwndDoc = hwndDoc;

  return QueryDocType(hwndDoc);
}

//
// int QueryDocType(HWND hwndDoc)
//
// returns the type of the document received in parameter
//
// This function MUST be updated for every new MDI document Class
//
int QueryDocType(HWND hwndDoc)
{
  int *p;

  // Emb 17/07/97: GetWindowLong on null hwnd fails under unix
  HWND hDocHandle = GetVdbaDocumentHandle(hwndDoc);
  if(hDocHandle)
    p = (int *)GetWindowLong(hDocHandle, 0);
  else
    p = 0;
  if (!p) {
    // TS_SendMessage seems useless in the case of invocation from IsImaActiveSession()
	// (it hangs as well in the case the animation hasn't appeared yet)
    // finally, the corresponding function call from Getsession() was replaced by some different code in dbaginfo.sc
    return SendMessage(hwndDoc, WM_USER_GETMFCDOCTYPE, 0, 0);
  }
  switch (*p) {
    case TYPEDOC_DOM:
      return TYPEDOC_DOM;
      break;
  default:
      return TYPEDOC_UNKNOWN;
  }
}

//
//  CloseAllChildren (VOID)
//
//  destroys all mdi windows
//
static VOID NEAR CloseAllChildren (BOOL bKeepNodeList)
{
  HWND hwndT;

  HWND hwndNodeList = 0;

  // Step 1: get Node List mdi doc window handle, if necessary
  if (bKeepNodeList) {
    HWND  hwndCurDoc;

    // get first document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);

    while (hwndCurDoc) {
      // test node selection window
      if (QueryDocType(hwndCurDoc) == TYPEDOC_NODESEL) {
        hwndNodeList = hwndCurDoc;
        break;
      }

      // get next document handle (with loop to skip the icon title windows)
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
      while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
        hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    }
  }

  // Step 2: delete all documents except Node List mdi doc if exists
  while(1) {
    hwndT = GetWindow (hwndMDIClient, GW_CHILD);
    // Skip the icon title windows
    while (hwndT && GetWindow (hwndT, GW_OWNER))
      hwndT = GetWindow (hwndT, GW_HWNDNEXT);
    if (hwndT == hwndNodeList) {
      // skip nodelist : get next, if exists
      hwndT = GetWindow (hwndT, GW_HWNDNEXT);
      while (hwndT && GetWindow (hwndT, GW_OWNER))
        hwndT = GetWindow (hwndT, GW_HWNDNEXT);
    }
    if (hwndT)
      SendMessage (hwndMDIClient, WM_MDIDESTROY, (WPARAM)hwndT, 0L);
    else
      break;
  }
}

//
//  InitializeApplication()
//
//    Registers the classes
//
static BOOL NEAR InitializeApplication(VOID)
{
  WNDCLASS  wc;

  // Register the frame class
  wc.style         = 0;
  wc.lpfnWndProc   = FrameWndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInst;
  wc.hIcon         = LoadIcon(hResource, MAKEINTRESOURCE(IDI_FRAME));
  wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
  wc.lpszMenuName  = NULL; /*MAKEINTRESOURCE(ID_MENU_MAINMENU);*/
  wc.lpszClassName = szFrame;

  if (!RegisterClass (&wc))
    return FALSE;

  // Register the MDI child classes

  // dom
  if (!DomRegisterClass())
    return FALSE;
  // all is well
  return TRUE;
}

//
//  InitializeInstance(LPSTR lpCmdLine, int nCmdShow)
//
//    Creates the frame window and an initial DOM window
//
// At this time, lpCmdLine not used
//
static BOOL NEAR InitializeInstance(LPSTR lpCmdLine, int nCmdShow)
{
  // dummy instructions to skip compiler warnings at level four
  lpCmdLine;

  // Initialize the font that will be used in the status bars
  // before creating the frame
  hFontStatus = ReadFontFromIni(IDS_INISETTINGS, IDS_INISTATUSBARFONT);

  // Same for font used by browsers (doms)
  hFontBrowsers = ReadFontFromIni(IDS_INISETTINGS,IDS_INIBROWSFONT);

  // Same for font used by Containers
  hFontPropContainers = ReadFontFromIni(IDS_INISETTINGS,IDS_INICONTFONT);

  hFontIpmTree  = ReadFontFromIni(IDS_INISETTINGS,IDS_INIIPMFONT);
  hFontNodeTree = ReadFontFromIni(IDS_INISETTINGS,IDS_ININODEFONT);

  // Don't create frame window if in library mode!!!
    // Initialize the "Select Window" box delayed in MainOnCreate
    // Initializes the node struct also delayed in MainOnCreate because of bkrefresh timer

    return TRUE;
}

//
//  TerminateInstance()
//
//    opposite of InitializeInstance
//
static VOID NEAR TerminateInstance()
{
  EndNodeStruct();

  // Free icons and bitmaps caches
  IconCacheFree();
  BitmapCacheFree();

  // Delete remaining GDI objects
  if (IsGDIObject(hFontStatus))
    DeleteObject(hFontStatus);

  if (IsGDIObject(hFontBrowsers))
    DeleteObject(hFontBrowsers);

  if (IsGDIObject(hFontPropContainers))
    DeleteObject(hFontPropContainers);

  if (IsGDIObject(hFontIpmTree))
    DeleteObject(hFontIpmTree);
  if (IsGDIObject(hFontNodeTree))
    DeleteObject(hFontNodeTree);

  FreeSqlErrorPtr();    // free SQL error linked list
}

//
//  MenuSelect
//
//  Manages the status bar message according to current menuitem,
//  and the Menu context help variable
//
static VOID NEAR MenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
  //
  // Help management
  //

  if (flags == 0)
    dwMenuHelpId = 0;     // menu closed
  else 
    if (hmenuPopup)
      dwMenuHelpId = 0;   // help on popups not implemented
    else
      if (item)
        dwMenuHelpId = item + OFFSETHELPMENU;
          // offset of 20000 for menuitems (convention with help writers)
      else
        dwMenuHelpId = 0;   // reset when closing menu (windows bug?)

      // special management for cut/copy/paste
      if (item==IDM_CUT || item==IDM_COPY || item==IDM_PASTE) {
        switch (QueryCurDocType(hwnd, NULL)) {
          case TYPEDOC_DOM:
            break;
          default:
            dwMenuHelpId += 800;
            break;
        }
      }
}


/* // */
/* // UK.S 11-AUG-1997: MAINWIN              */
/* // Make sure to delete the temporary file */
/* // */
static void UNIX_LoadCleanup(const char* f1, const char* f2)
{
#if defined (MAINWIN)
  if (access (f1, F_OK) == 0)
      if (remove (f1) == -1)
          return;
  if (access (f2, F_OK) == 0)
      if (remove (f2) == -1)
          return; 
#endif
}
//
// Manages the "Open" menuitem
//
static BOOL  NEAR OpenEnvironment(HWND hwnd, LPSTR lpszCmdLine)
{
  BOOL          bRet;
  TOOLSOPENFILE toolsopenfile;
  int           ret;  // RES_xxx
  FILEIDENT     idFile;
  BOOL          bStopHere;      // in case we stop at password check time

  // "store current environment" variables
  char          buf[BUFSIZE];
  char          buf2[BUFSIZE];
  char          rsCapt[BUFSIZE];
  char          rsBuf[BUFSIZE];
  char          bufEnv[BUFSIZE];
  int           mbRet;
  BOOL          bNoOpen;
#ifdef MAINWIN
  char		cmd[1024];
  CL_ERR_DESC cl_err;
#endif


  char secondFile     [164];
  char uncompressFile [164];

  // initial load: don't call the dialog box
  if (lpszCmdLine) {
    x_strcpy(toolsopenfile.szFile, lpszCmdLine);
    bRet = TRUE;      // simulate successful dialog
  }
  else {
    // Request confirmation from the user if might be useful
    bNoOpen = FALSE;     // default
    if (bSaveRecommended) {
      LoadString(hResource, IDS_NEW_QUERYSAVE_TXT, rsBuf, sizeof(rsBuf));
      if (envPathName[0])
    x_strcpy(bufEnv, envPathName);
      else
    LoadString(hResource, IDS_NEW_UNTITLED_ENV, bufEnv, sizeof(bufEnv));
      wsprintf(buf, rsBuf, bufEnv);
      LoadString(hResource, IDS_NEW_QUERYSAVE_CAPT, rsCapt, sizeof(rsCapt));
	  wsprintf(buf2,rsCapt,VDBA_GetInstallName4Messages());
      mbRet = MessageBox(hwndFrame, buf, buf2,
            MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL);
      switch (mbRet) {
    case IDCANCEL:
      bNoOpen = TRUE;
      break;
    case IDNO:
      bNoOpen = FALSE;
      break;
    case IDYES:
      if (envPathName[0]) {
        SaveEnvironment(hwnd, (LPSTR)NULL, (LPSTR)NULL, FALSE);
        bNoOpen = FALSE;
      }
      else {
        if (SaveAsEnvironment(hwnd))
          bNoOpen = FALSE;
        else
          bNoOpen = TRUE;
      }
      }
    }

    // don't open ?
    if (bNoOpen)
      return FALSE;

    // Dialog box for open
    memset(&toolsopenfile, 0, sizeof(toolsopenfile));
    bRet = DlgOpenFile(hwnd, &toolsopenfile);
  }

  // open environment if requested
  if (bRet) {

    // hour glass to show intense system activity
    ShowHourGlass();

    // Clean the previous environment and initialize a new one
    CloseAllChildren (FALSE);

    EndNodeStruct();
    initNodeStruct();

    // In case load fails, to avoid "system 7 error" endless loop
    MfcResetNodesTree();

    envPathName[0] = '\0';
    UpdateAppTitle();
    /* // */
    /* // UK.S 11-AUG-1997: MAINWIN                                                 */
    /* // uncompress uses the file has .Z, so we create the second file, that is    */
    /* // the original file with extention .vdbatemp.Z                              */
    /* // Next, uncompress that second file for reading and then destroy the file.  */
    /* // Remark: Cannot use tempnam to create temporary file with the extention    */
    /* // */
#if defined (MAINWIN)
    sprintf (secondFile,     "%s.vdbatemp.Z", toolsopenfile.szFile);
    sprintf (uncompressFile, "%s.vdbatemp",   toolsopenfile.szFile);
    if (access (secondFile, F_OK) == 0)
        if (remove (secondFile) == -1)
            return FALSE;
    if (access (uncompressFile, F_OK) == 0)
        if (remove (uncompressFile) == -1)
            return FALSE;

    STprintf( cmd, "cp %s %s", (const char*)toolsopenfile.szFile,
	(const char*)secondFile);
    if( PCcmdline( (LOCATION *) NULL, cmd ,
	PC_WAIT, (LOCATION *) NULL, &cl_err ) != OK )
    {
        UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
        return FALSE;
    }

    STprintf( cmd, "uncompress %s", (const char*)secondFile);
    if( PCcmdline( (LOCATION *) NULL, cmd ,
	PC_WAIT, (LOCATION *) NULL, &cl_err ) != OK )
    {
        UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
        return FALSE;
    }
    ret = DBAFileOpen4Read((LPUCHAR)uncompressFile, &idFile);
#else
    ret = DBAFileOpen4Read((LPUCHAR)(toolsopenfile.szFile), &idFile);
#endif
    if (ret!=RES_SUCCESS) {
      RemoveHourGlass();
      ErrorMsgBox(IDS_ERR_LOAD_OPEN);
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    ret = DOMLoadVersionMarker(idFile);
    if (ret!=RES_SUCCESS) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      ErrorMsgBox(IDS_ERR_LOAD_VERSIONID);
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    ret = DOMLoadPassword(idFile, &bStopHere);
    if (bStopHere) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }
    if (ret!=RES_SUCCESS) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      ErrorMsgBox(IDS_ERR_LOAD_PASSWORD);
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    ret = DOMLoadSettings(idFile);
    if (ret!=RES_SUCCESS) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      ErrorMsgBox(IDS_ERR_LOAD_SETTINGS);
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    gMemErrFlag = MEMERR_NOMESSAGE;
    ret = DOMLoadCache(idFile);
    if (ret!=RES_SUCCESS) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                   ResourceString(IDS_E_MEMERR_LOADCACHE),
                   NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      else {
        if (ret==RES_WRONGLOCALNODE) {
			char buftemp[280];
			char bufext[200];
			fstrncpy(buftemp, ResourceString ((UINT)IDS_CFGFILES_FILTER),sizeof(buftemp)-1);
			if (buftemp[0]!='*' || buftemp[1]!='.' || x_strlen(buftemp)>20) {
				myerror(ERR_FILE); // internal error
				x_strcpy(bufext,"");
			}
			else 
				wsprintf(bufext,buftemp+1);
			wsprintf(buftemp,ResourceString((UINT)IDS_LOADSAVE_LOCALSERVER_CHANGED),bufext);
			MessageBox(GetFocus(),buftemp,
              NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
        }
        ErrorMsgBox(IDS_ERR_LOAD_CACHE);
      }
      gMemErrFlag = MEMERR_STANDARD;
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    gMemErrFlag = MEMERR_NOMESSAGE;

    ret = DOMLoadDisp(idFile);
    if (ret!=RES_SUCCESS) {
      DBACloseFile(idFile);
      RemoveHourGlass();
      if (gMemErrFlag == MEMERR_HAPPENED)
        MessageBox(GetFocus(),
                   ResourceString(IDS_E_MEMERR_LOADDISP),
                   NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      else
        ErrorMsgBox(IDS_ERR_LOAD_DISP);
      gMemErrFlag = MEMERR_STANDARD;
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    gMemErrFlag = MEMERR_STANDARD;

    ret = DBACloseFile(idFile);
    if (ret!=RES_SUCCESS) {
      RemoveHourGlass();
      ErrorMsgBox(IDS_ERR_LOAD_CLOSE);
      UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
      return FALSE;
    }

    // Update the global environment variables
    x_strcpy(envPathName, toolsopenfile.szFile);

    gMemErrFlag = MEMERR_NOMESSAGE;
    RefreshOnLoad();   // manages the 'refresh on load' parameters 
    if (gMemErrFlag == MEMERR_HAPPENED) {
      MessageBox(GetFocus(),
                 ResourceString(IDS_E_MEMERR_REFRESHONLOAD),
                 NULL, MB_OK | MB_ICONSTOP | MB_TASKMODAL);
      bRet = FALSE;
    }
    gMemErrFlag = MEMERR_STANDARD;

    // remove guess what
    RemoveHourGlass();
  }
  UNIX_LoadCleanup ((const char*)secondFile, (const char*)uncompressFile);
  return bRet;
}

//
// Manages the "Save as" menuitem
//
static BOOL NEAR SaveAsEnvironment(HWND hwnd)
{
  BOOL          bRet;
  TOOLSSAVEFILE toolssavefile;

  memset(&toolssavefile, 0, sizeof(toolssavefile));
  bRet = DlgSaveFile(hwnd, &toolssavefile);
  if (bRet) {
    if (SaveEnvironment(hwnd, toolssavefile.szFile,
                              toolssavefile.szPassword,
                              toolssavefile.bPassword)) {
      bEnvPassword = toolssavefile.bPassword;
      x_strcpy(envPassword, toolssavefile.szPassword);
      return TRUE;
    }
    else
      return FALSE;
  }
  return bRet;
}

//
// Manages the "Save" menuitem
//
static BOOL NEAR SaveEnvironment(HWND hwnd, char *envName, char *passwordName, BOOL bPassword)
{
  int           ret;  // RES_xxx
  FILEIDENT     idFile;
#ifdef MAINWIN
  char          cmd[1024];
  CL_ERR_DESC cl_err;
#endif

#ifdef MAINWIN  
  char secondFile     [164];
#endif  

  // manage called from "Save As"
  if (envName==NULL) {
    envName = envPathName;
    passwordName = envPassword;
    bPassword = bEnvPassword;
  }

  // manage empty name
  if (envName[0] == '\0')
    return SaveAsEnvironment(hwnd);

  // Here we can save!
  ShowHourGlass();

  ret = DBAFileOpen4Save((LPUCHAR)envName, &idFile);
  if (ret!=RES_SUCCESS) {
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_OPEN);
    return FALSE;
  }

  bSaveCompressed = TRUE;

  ret = DOMSaveVersionMarker(idFile);
  if (ret!=RES_SUCCESS) {
    DBACloseFile(idFile);
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_VERSIONID);
    return FALSE;
  }

  ret = DOMSavePassword(idFile, passwordName, bPassword);
  if (ret!=RES_SUCCESS) {
    DBACloseFile(idFile);
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_PASSWORD);
    return FALSE;
  }

  ret = DOMSaveSettings(idFile);
  if (ret!=RES_SUCCESS) {
    DBACloseFile(idFile);
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_SETTINGS);
    return FALSE;
  }

  ret = DOMSaveCache(idFile);
  if (ret!=RES_SUCCESS) {
    DBACloseFile(idFile);
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_CACHE);
    return FALSE;
  }


  ret = DOMSaveDisp(idFile);
  if (ret!=RES_SUCCESS) {
    DBACloseFile(idFile);
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_DISP);
    return FALSE;
  }

  ret = DBACloseFile(idFile);
  if (ret!=RES_SUCCESS) {
    unlink(envName);
    RemoveHourGlass();
    ErrorMsgBox(IDS_ERR_SAVE_CLOSE);
    return FALSE;
  }

  // All is well
  x_strcpy(envPathName, envName);
  /* // */
  /* // UK.S 11-AUG-1997: MAINWIN                                                                 */
  /* // Compress the file by using compress. compress changes the file by adding the extention .Z */
  /* // Next, rename the file to the original without extention .Z,                               */
  /* //       change file attributes to read-write only for the owner.                            */
  /* // */
#if defined (MAINWIN)
  sprintf (secondFile,     "%s.Z", envName);
  if (access (secondFile, F_OK) == 0)
      if (remove (secondFile) == -1)
          return FALSE;
  STprintf( cmd, "compress %s", (const char*)envName );
  if( PCcmdline( (LOCATION *) NULL, cmd ,
	PC_WAIT, (LOCATION *) NULL, &cl_err ) != OK )
  {
	if (access (secondFile, F_OK) == 0)
	    remove (secondFile);
	return FALSE;
  }
  if (rename (secondFile, envName) == -1)
      return FALSE;
  STprintf( cmd, "chmod 0600 %s", (const char*)envName );
  PCcmdline( (LOCATION *) NULL, cmd ,
	PC_WAIT, (LOCATION *) NULL, &cl_err );
#endif
  RemoveHourGlass();
  return TRUE;
}

//
// Updates the application title by adding the current environment
// name if any.
//
static VOID NEAR UpdateAppTitle(void)
{
  char captBuf[BUFSIZE];
  char buf[2*BUFSIZE];
  char buf2[BUFSIZE];

  VDBA_GetProductCaption(captBuf, sizeof(captBuf));
  if (envPathName[0] != '\0') {
    x_strcpy(buf2, envPathName);
    AnsiLower(buf2);
    wsprintf(buf, "%s - %s", captBuf, buf2);
  }
  else
    x_strcpy(buf, captBuf);
  SetWindowText(hwndFrame, buf);
}

//
//  Code for the modeless "Find" dialog box
//
BOOL ProcessFindDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (toolsfind.uMessage == message) {
    if (wParam & FIND_DIALOGTERM) {
      hDlgSearch = NULL;
      memset(&toolsfind, 0, sizeof(toolsfind));
      EnableMenuItem(GetMenu(hwndFrame), IDM_FIND,
                     MF_BYCOMMAND | MF_ENABLED);
      NanBar_Enable(hwndToolBar, IDM_FIND);     // Added Emb Jan 15, 97
      return TRUE;
    }

    if (wParam & FIND_FINDNEXT) {
      HWND hwndDoc;
      int  docType;

      // get the find operation performed in the current mdi doc
      // wParam will have to be cracked as follows:
      // BOOL bMatchCase = (wParam & FIND_MATCHCASE ? TRUE : FALSE);
      // BOOL bWholeWord = (wParam & FIND_WHOLEWORD ? TRUE : FALSE);
      docType = QueryCurDocType(hwnd, &hwndDoc);
      switch(docType) {
        case TYPEDOC_UNKNOWN:
        case TYPEDOC_NONE:
          break;
        case TYPEDOC_DOM:
          SendMessage(GetVdbaDocumentHandle(hwndDoc), WM_USERMSG_FIND, wParam, (LPARAM)&szFind);
          break;
        default:    // MFC docs, including nodesel in window mode
          SendMessage(hwndDoc, WM_USERMSG_FIND, wParam, (LPARAM)&szFind);
          break;
      }
    }
    return TRUE;
  }
  return FALSE;
}

//----------------------------------------------------------------------
//
//                          Save/Load functions
//
//----------------------------------------------------------------------

//
// environment security management
//

static int  NEAR DOMSavePassword(FILEIDENT idFile, char *passwordName, BOOL bPassword)
{
  int retval;

  // TO BE FINISHED - CRYPT ALGORITHM
  retval = DBAAppend4Save(idFile, (void *)&bPassword, sizeof(bPassword));
  if (retval != RES_SUCCESS)
    return retval;
  retval = DBAAppend4Save(idFile, (void *)passwordName, PASSWORD_LEN);
  if (retval != RES_SUCCESS)
    return retval;

  return RES_SUCCESS;
}

//
//  Dialog proc for the box that manages the verify environment password
//
BOOL WINAPI __export VerifyEnvPasswordDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg) {
    HANDLE_MSG(hDlg, WM_INITDIALOG, VerifyOnInitDialog);
    HANDLE_MSG(hDlg, WM_COMMAND, VerifyOnCommand);
  }
  return FALSE;
}

static BOOL VerifyOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
  if (!HDLGSetProp(hDlg,(LPVOID) lParam))
    EndDialog(hDlg,FALSE);
  CenterDialog(hDlg);
  SendDlgItemMessage(hDlg, IDD_LOADENV_PASSWORD, EM_LIMITTEXT,
                         PASSWORD_LEN-1, 0L);
  //SendDlgItemMessage(hDlg, IDD_LOADENV_CONFIRMPASSWORD, EM_LIMITTEXT,
  //                                                 PASSWORD_LEN-1, 0L);
  return TRUE;
}

static void VerifyOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
  char psw1[PASSWORD_LEN];
  //char psw2[PASSWORD_LEN];
  char rsBuf[BUFSIZE];
  char *lpPassword;

  switch (id) {
    case IDOK:
      GetDlgItemText(hDlg, IDD_LOADENV_PASSWORD, psw1, sizeof(psw1)-1);
      //GetDlgItemText(hDlg, IDD_LOADENV_CONFIRMPASSWORD, psw2, sizeof(psw2)-1);
      //if (x_strcmp(psw1, psw2)) {
      //  LoadString(hResource, (UINT)IDS_E_PLEASE_RETYPE_PASSWORD,
      //            rsBuf, sizeof(rsBuf)-1);
      //  MessageBox(hDlg, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
      //  SetFocus(hDlg);
      //  return TRUE;    // Don't exit
      //}
      if (!x_strlen(psw1)) {
    LoadString(hResource, (UINT)IDS_E_PASSWORD_REQUIRED,
          rsBuf, sizeof(rsBuf)-1);
    MessageBox(hDlg, rsBuf, NULL, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
    SetFocus(hDlg);
    return;    // Don't exit
      }
      lpPassword = (char *)HDLGGetProp(hDlg);
      x_strcpy(lpPassword, psw1);
      HDLGRemoveProp(hDlg);
      EndDialog(hDlg, TRUE);
      break;

    case IDCANCEL:
      HDLGRemoveProp(hDlg);
      EndDialog(hDlg, FALSE);
      break;
  }
}

static int  NEAR DOMLoadPassword(FILEIDENT idFile, BOOL *pbStopHere)
{
  int     retval;
  char    filePassword[PASSWORD_LEN];
  char    kbdPassword[PASSWORD_LEN];
  BOOL    bFilePassword;
  DLGPROC lpfnDlg;

  // TO BE FINISHED - DECRYPT ALGORITHM

  *pbStopHere = FALSE;

  retval = DBAReadData(idFile, (void *)&bFilePassword, sizeof(bFilePassword));
  if (retval != RES_SUCCESS)
    return retval;
  retval = DBAReadData(idFile, (void *)&filePassword, PASSWORD_LEN);
  if (retval != RES_SUCCESS)
    return retval;

  // No password
  if (!bFilePassword) {
    bEnvPassword = FALSE;
    envPassword[0] = '\0';
    return RES_SUCCESS; // No password
  }

  // loop on box until cancel or good password
  while (1) {
    lpfnDlg = (DLGPROC) MakeProcInstance((FARPROC)VerifyEnvPasswordDlgProc, hInst);
    lpHelpStack = StackObject_PUSH (lpHelpStack,
                        StackObject_INIT ((UINT)VERIFYENVPASSWORDDIALOG));
    retval = VdbaDialogBoxParam(hResource,
                MAKEINTRESOURCE(VERIFYENVPASSWORDDIALOG),
                GetFocus(),
                lpfnDlg,
                (LPARAM)(&kbdPassword));
    lpHelpStack = StackObject_POP (lpHelpStack);
    FreeProcInstance ((FARPROC) lpfnDlg);

    if (retval != IDOK) {
      *pbStopHere = TRUE;
      return RES_SUCCESS;
    }

    // check consistency
    if (lstrcmp(filePassword, kbdPassword)) {
      *pbStopHere = TRUE;
      ErrorMsgBox(IDS_ERR_LOAD_BADPASSWORD);
    }
    else
      break;    // leave the while loop
  }

  // store in global variables
  bEnvPassword = bFilePassword;
  if (bEnvPassword)
    x_strcpy(envPassword, kbdPassword);
  else
    envPassword[0] = '\0';
  *pbStopHere = FALSE;
  return RES_SUCCESS;
}

// Save/Load structure for the application
typedef struct _tagAppInfo {
  int   x;                  // horizontal position
  int   y;                  // vertical position
  int   cx;                 // width
  int   cy;                 // height
  BOOL  bZoom;              // TRUE if application maximized
  BOOL  bBkRefreshOn;       // is background refresh enabled ?
} APPINFO, FAR *LPAPPINFO;

int DOMSaveSettings(FILEIDENT idFile)
{
  int retval = MfcSaveSettings(idFile);
  return retval;
}

int DOMLoadSettings(FILEIDENT idFile)
{
  int retval = MfcLoadSettings(idFile);
  return retval;
}

int DOMSaveDisp(FILEIDENT idFile)
{
  int               nbDocs;
  HWND              hwndCurDoc;
  int               retval;
  WINDOWPLACEMENT   wndpl;
  HWND              hwndMaxDoc;
  int               docType;
  COMMONINFO        sDocInfo;

  // specific for DOM document save
  LPDOMDATA         lpDomData;
  HWND              hTree;
  DWORD             curRecId;
  LPTREERECORD      lpRecord;
  LPSTR             caption;
  DOMDATAINFO       sDomDataInfo;
  COMPLIMTREE       sComplim;
  COMPLIMTREELINE   sComplimLine;

  // specific for size/pos of the application
  APPINFO           sAppInfo;
  RECT              rc;

  // Initializations
  hwndMaxDoc = 0;

  // Step 0: size and position of the application,
  // plus flag bBkRefreshOn
  GetWindowRect(hwndFrame, &rc);
  sAppInfo.x  = rc.left;
  sAppInfo.y  = rc.top;
  sAppInfo.cx = rc.right - rc.left;
  sAppInfo.cy = rc.bottom - rc.top;
  sAppInfo.bZoom = IsZoomed(hwndFrame);
  sAppInfo.bBkRefreshOn = bBkRefreshOn;
  retval = DBAAppend4Save(idFile, (void *)(LPAPPINFO)&sAppInfo,
                  sizeof(APPINFO));
  if (retval != RES_SUCCESS)
    return retval;

  // Step 1: number of documents to be saved
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  nbDocs = 0;
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  while (hwndCurDoc) {
    docType = QueryDocType(hwndCurDoc);
    if (docType==TYPEDOC_DOM /*|| docType==TYPEDOC_PROPERTY*/
                 /*|| docType==TYPEDOC_SQLACT*/) 
      nbDocs++;

    // store also mfc documents
    if (docType == TYPEDOC_MONITOR || docType == TYPEDOC_DBEVENT || docType == TYPEDOC_NODESEL || docType==TYPEDOC_SQLACT)
      nbDocs++;
    // get next document handle (with loop to skip the icon title windows)
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDNEXT);
  }
  retval = DBAAppend4Save(idFile, (void *)&nbDocs, sizeof(nbDocs));
  if (retval != RES_SUCCESS)
    return retval;

  // Step 2: for each document : several steps
  // We loop through the docs in reverse order to preserve Z order at load
  hwndCurDoc = GetWindow (hwndMDIClient, GW_CHILD);
  hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDLAST);
  while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDPREV);
  while (hwndCurDoc) {

    docType = QueryDocType(hwndCurDoc);

    // Step 2.1: save Type/Size/Pos/Iconstate/Caption
    if (docType == TYPEDOC_DOM /*|| docType == TYPEDOC_PROPERTY*/
                  /* || docType==TYPEDOC_SQLACT*/) {

      retval = DBAAppend4Save(idFile, (void *)&docType, sizeof(docType));
      if (retval != RES_SUCCESS)
        return retval;

      GetWindowText(hwndCurDoc, sDocInfo.szTitle, sizeof(sDocInfo.szTitle)-1);
      wndpl.length = sizeof(wndpl);   // Microsoft Nerdy stuff - see doc.
      if (GetWindowPlacement(hwndCurDoc, (WINDOWPLACEMENT FAR *)&wndpl)) {
        sDocInfo.x  = wndpl.rcNormalPosition.left;
        sDocInfo.y  = wndpl.rcNormalPosition.top;
        sDocInfo.cx = wndpl.rcNormalPosition.right  - sDocInfo.x;
        sDocInfo.cy = wndpl.rcNormalPosition.bottom - sDocInfo.y;
      }
      else
        sDocInfo.x = sDocInfo.y = sDocInfo.cx = sDocInfo.cy = CW_USEDEFAULT;
      sDocInfo.bIcon = IsIconic(hwndCurDoc);
      sDocInfo.bZoom = IsZoomed(hwndCurDoc);
      if (sDocInfo.bZoom)
        hwndMaxDoc = hwndCurDoc;
      switch(docType) {
        case TYPEDOC_DOM:
          lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
          sDocInfo.ingresVer   = lpDomData->ingresVer;
          break;
      }
      retval = DBAAppend4Save(idFile, (void *)(LPCOMMONINFO)&sDocInfo,
                      sizeof(COMMONINFO));
      if (retval != RES_SUCCESS)
        return retval;
    }

    // step 2.1 for mfc documents
    if (docType == TYPEDOC_MONITOR || docType == TYPEDOC_DBEVENT || docType == TYPEDOC_NODESEL || docType == TYPEDOC_SQLACT) {
      retval = DBAAppend4Save(idFile, (void *)&docType, sizeof(docType));
      if (retval != RES_SUCCESS)
        return retval;

    }


    // Step 2.2 to 2.x : save the document-related data
    switch (docType) {
      case TYPEDOC_DOM:
        // Step 2.2: save the Domdata structure important elements
        lpDomData = (LPDOMDATA)GetWindowLong(GetVdbaDocumentHandle(hwndCurDoc), 0);
        sDomDataInfo.nodeHandle   = lpDomData->psDomNode->nodeHandle;
        sDomDataInfo.currentRecId = lpDomData->currentRecId;
        sDomDataInfo.bTreeStatus  = lpDomData->bTreeStatus;
        sDomDataInfo.bwithsystem  = lpDomData->bwithsystem;
        x_strcpy(sDomDataInfo.lpBaseOwner, lpDomData->lpBaseOwner); // dummy for Ingres Desktop
        x_strcpy(sDomDataInfo.lpOtherOwner, lpDomData->lpOtherOwner);
        sDomDataInfo.domType  = lpDomData->domType;
        sDomDataInfo.domNumber= lpDomData->domNumber;
        //sDomDataInfo.gatewayType = lpDomData->gatewayType;
        retval = DBAAppend4Save(idFile, (void *)(LPDOMDATAINFO)&sDomDataInfo,
                        sizeof(DOMDATAINFO));
        if (retval != RES_SUCCESS)
          return retval;

        // Step 2.3: save number of items, selected and first visible line
        hTree = lpDomData->hwndTreeLb;
        sComplim.count_all = (long)SendMessage(hTree, LM_GETCOUNT, COUNT_ALL, 0L);
        sComplim.curRecId  = GetCurSel(lpDomData);
        sComplim.FirstVis  = (DWORD)SendMessage(hTree, LM_GETTOPITEM, 0, 0L);
        retval = DBAAppend4Save(idFile,
                (void *)(LPCOMPLIMTREE)&sComplim,
                sizeof(COMPLIMTREE));
        if (retval != RES_SUCCESS)
          return retval;

        // Step 2.4: save the items of the tree
        curRecId = SendMessage(hTree, LM_GETFIRST, 0, 0L);
        while (curRecId) {
          // 2.4.1: Save tree record
          lpRecord = (LPTREERECORD) SendMessage(hTree, LM_GETITEMDATA,
                        0, (LPARAM)curRecId);
          retval = DBAAppend4Save(idFile,
                  (void *)lpRecord, sizeof(TREERECORD));
          if (retval != RES_SUCCESS)
            return retval;

          // 2.4.2: Save tree line caption, id, parent id, opening state

          memset(&sComplimLine, 0, sizeof(sComplimLine));

          caption = (LPSTR) SendMessage(hTree, LM_GETSTRING,
                    0, (LPARAM)curRecId);
          x_strcpy(sComplimLine.caption, caption);
          sComplimLine.recId = curRecId;
          sComplimLine.parentRecId = (DWORD)SendMessage(hTree, LM_GETPARENT,
                            0, (LPARAM)curRecId);
          sComplimLine.recState = (int)SendMessage(hTree, LM_GETSTATE,
                          0, (LPARAM)curRecId);

          retval = DBAAppend4Save(idFile,
                  (void *)(LPCOMPLIMTREELINE)&sComplimLine,
                  sizeof(COMPLIMTREELINE));
          if (retval != RES_SUCCESS)
            return retval;

          // Loop for next record
          curRecId = SendMessage(lpDomData->hwndTreeLb, LM_GETNEXT, 0, 0L);
        }
        break;

      case TYPEDOC_MONITOR:
        if (!SaveNewMonitorWindow(hwndCurDoc))
          return RES_ERR;
        break;

      case TYPEDOC_DBEVENT:
        if (!SaveNewDbeventWindow(hwndCurDoc))
          return RES_ERR;
        break;
      case TYPEDOC_SQLACT:
        if (!SaveNewSqlactWindow(hwndCurDoc))
          return RES_ERR;
        break;

      case TYPEDOC_NODESEL:
        if (!SaveNewNodeselWindow(hwndCurDoc))
          return RES_ERR;
        break;

      default:
        ASSERT (FALSE);
        return RES_ERR;


    }   // End of switch

    // for ex-non mfc documents only (dom, sqlact, property):
    // serialise special data which will be reloaded
    // by OnInitialUpdate() view method
    switch (docType) {
      case TYPEDOC_DOM:
        if (!SaveExNonMfcSpecialData(hwndCurDoc))
          return RES_ERR;
    }

    // Search for the next MDI document
    hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDPREV);
    while (hwndCurDoc && GetWindow (hwndCurDoc, GW_OWNER))
      hwndCurDoc = GetWindow (hwndCurDoc, GW_HWNDPREV);
  }

  // Step 3: store the handle of the mdi doc that was in maximize mode
  retval = DBAAppend4Save(idFile, (void *)&hwndMaxDoc, sizeof(hwndMaxDoc));
  if (retval != RES_SUCCESS)
    return retval;

  return RES_SUCCESS;
}

int DOMLoadDisp(FILEIDENT idFile)
{
  int     nbDocs;
  int     cptDoc;
  int     retval;
  HWND    hwndMaxDoc;
  int     docType;
  APPINFO sAppInfo;

  HMENU hMenu;

  hMenu = GetMenu(hwndFrame);

  // Step 0: size and position of the application
  retval = DBAReadData(idFile, (void *)(LPAPPINFO)&sAppInfo,
                   sizeof(APPINFO));
  if (retval != RES_SUCCESS)
    return retval;
  if (sAppInfo.bZoom == TRUE)
    ShowWindow(hwndFrame, SW_SHOWMAXIMIZED);
  else {
    ShowWindow(hwndFrame, SW_RESTORE);  // in case is currently maximized
    MoveWindow(hwndFrame, sAppInfo.x,  sAppInfo.y,
                          sAppInfo.cx, sAppInfo.cy, TRUE);
  }

  // bkrefresh flag delayed til' the end of the load process
  // to correctly update the menus
  // Step 1: read the number of saved MDI documents
  retval = DBAReadData(idFile, (void *)&nbDocs, sizeof(nbDocs));
  if (retval != RES_SUCCESS)
    return retval;

  // Step 1 bis: reset sequential numbers for mdi docs captions
  seqDomNormal      = 0;    // normal dom sequential number
  seqDomTearOut     = 0;    // teared out dom sequential number
  seqDomRepos       = 0;    // repositioned dom sequential number
  seqDomScratchpad  = 0;    // scratchpad dom sequential number

  // Step 2: for each document : Create it if type acceptable
  for (cptDoc = 0; cptDoc < nbDocs; cptDoc++) {
    retval = DBAReadData(idFile, (void *)&docType, sizeof(docType));
    if (retval != RES_SUCCESS)
      return retval;

    switch (docType) {
      case TYPEDOC_DOM:
        // Make a DOM type MDI child window
        if (MakeNewDomMDIChild (DOM_CREATE_LOAD, 0, idFile, 0) == NULL)
          return RES_ERR;
        break;
      case TYPEDOC_MONITOR:          // Monitor window   
        if (!OpenNewMonitorWindow())
          return RES_ERR;
        break;

      case TYPEDOC_DBEVENT:          // Database events trace window
        if (!OpenNewDbeventWindow())
          return RES_ERR;
        break;
      case TYPEDOC_SQLACT:          // Sqlact window
        if (!OpenNewSqlactWindow())
          return RES_ERR;
        break;

      case TYPEDOC_NODESEL:          // Nodes selection in non-docking view mode
        if (!OpenNewNodeselWindow())
          return RES_ERR;
        break;

      default :
        return RES_ERR;
    }
  }

  // step 2 plus : background refresh enabled/disabled
  // FOR ALL MENUS ON DOCUMENTS!
  if (bBkRefreshOn != sAppInfo.bBkRefreshOn) {
    LPUCHAR lpsaved,lpcur;
    UCHAR buf[600];
    int iret;
    if (sAppInfo.bBkRefreshOn)
      lpsaved=ResourceString(IDS_ON_);
    else
      lpsaved=ResourceString(IDS_OFF_);
    if (bBkRefreshOn)
      lpcur=ResourceString(IDS_ON_);
    else
      lpcur=ResourceString(IDS_OFF_);
    //"Your current setting for Background Refresh is %s,\n"
    //"but the environment you are loading was saved with Background Refresh %s.\n"
    //"Do you want to set it %s ?"
    wsprintf(buf,ResourceString(IDS_F_SETTING_BACKGROUND_REFRESH) ,lpcur,lpsaved,lpsaved);
    PushVDBADlg();
    //"Background Refresh Setting"
    iret = MessageBox(GetFocus(),buf,ResourceString(IDS_TITLE_SETTING_BACKGROUND_REFRESH),
                                 MB_ICONQUESTION | MB_YESNO |MB_TASKMODAL);
    PopVDBADlg();
    if (iret == IDYES) 
      bBkRefreshOn = sAppInfo.bBkRefreshOn;
  }
  if (bBkRefreshOn) {
    CheckMenuItem(hMenu,       IDM_CHMODREFRESH, MF_BYCOMMAND | MF_CHECKED);
    CheckMenuItem(hGwNoneMenu, IDM_CHMODREFRESH, MF_BYCOMMAND | MF_CHECKED);
  }
  else {
    CheckMenuItem(hMenu,       IDM_CHMODREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(hGwNoneMenu, IDM_CHMODREFRESH, MF_BYCOMMAND | MF_UNCHECKED);
  }

  // Step 3: in case there were several mdi documents,
  // force the maximize mode for the one that was in that state at save time
  retval = DBAReadData(idFile, (void *)&hwndMaxDoc, sizeof(hwndMaxDoc));
  if (retval != RES_SUCCESS)
    return retval;

  // Masqued because GPF!
  //if (hwndMaxDoc)
  //  SendMessage(hwndMDIClient, WM_MDIMAXIMIZE, (WPARAM)hwndMaxDoc, 0L);

  return RES_SUCCESS;
}

//
// Probably temporary -Version marker checker functions
//
static int  NEAR DOMSaveVersionMarker(FILEIDENT idFile)
{
  int   retval;
  char  buf[BUFSIZE];

  LoadString(hResource, IDS_VERSION_MARKER, buf, sizeof(buf)-1);

  buf[VERSIONSIZE-COMPRESSKEYSIZE] = '\0';

  if (bSaveCompressed)
  {
     x_strcat(buf, ":CAZIP");
#ifdef WIN32
     x_strcat(buf, ":32");
#else
     x_strcat(buf, ":16");
#endif
  }

  buf[VERSIONSIZE] = '\0';
  retval = DBAAppend4SaveNC(idFile, (void *)(LPSTR)&buf, VERSIONSIZE);
  return retval;
}

static int NEAR DOMLoadVersionMarker(FILEIDENT idFile)
{
  int   retval;
  char  buf[BUFSIZE];
  char  buf2[BUFSIZE];
  char * lpszZip;

  // prepare expected version buffer
  LoadString(hResource, IDS_VERSION_MARKER, buf, sizeof(buf)-1);

  buf[VERSIONSIZE] = '\0';

  // read stored version
  buf2[0] = '\0';
  retval = DBAReadData(idFile, (void *)(LPSTR)&buf2, VERSIONSIZE);
  if (retval != RES_SUCCESS)
    return retval;
  buf2[VERSIONSIZE] = 0;

  // search for the compression identifier

  if (lpszZip = x_strchr(buf2, ':'))
  {
      char * lpszPlatform;

      *lpszZip = (char)NULL;
      lpszZip++;
      // look for the platform identifier

      lpszPlatform = x_strchr(lpszZip, ':');
      if (lpszPlatform)
      {
#ifdef WIN32
          char szValidPlatform[] = "32";
#else
          char szValidPlatform[] = "16";
#endif

          *lpszPlatform = (char)NULL;
          lpszPlatform++;

          if (x_strcmp(lpszPlatform, szValidPlatform) != 0)
          {
              char szMessage[128];
              char szTitle[128];

              // We cannot load this CFG file.  Wrong platform.

              LoadString(hResource, IDS_INVALIDCFGPLATFORM, szMessage, sizeof(szMessage));
              VDBA_GetBaseProductCaption(szTitle, sizeof(szTitle));
              MessageBox(hwndFrame, szMessage, szTitle,
                         MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
              return RES_ERR;
          }
      }

      // check for the zipped file identifier

      if (x_strcmp(lpszZip, "CAZIP") == 0)
      {
          bReadCompressed = TRUE;
      }
  }

  if (x_strcmp(buf2, buf))
    return RES_ERR;
  else
    return RES_SUCCESS;
}

//----------------------------------------------------------------------
//
//                     Settings/Preferences functions
//
//    TO BE FINISHED : MERGE WITH DIALOG BOXES AND .INI FILE
//
//----------------------------------------------------------------------

//
// Reflects the current setting for the Nan Bar visibility
//
BOOL QueryShowNanBarSetting(int typedoc)
{
  return bShowNanBar;
}

//
// Reflects the current setting for the Status Bar visibility
//
BOOL QueryShowStatusSetting(int typedoc)
{
  return bShowStatus;
}


//--------------------------------------------------------------------------
//
// Global Status bar functions
//
// This section is derived from dom.c ribbon functions for the tree status
//
//--------------------------------------------------------------------------



//
//    Creates and initializes the global status bar
//
//  Parameters:
//    None
//
//  Returns:
//    The handle to the global status bar window if successful, otherwise NULL.
//
//
// Changed Emb : only initializes the fields sizes (if necessary),
//  and fills the fields with captions or empty text
//


//
//  Updates the global status bar - updates only if global status in use
//
VOID UpdateGlobalStatus()
{
  char    buf[BUFSIZE];
  int     nbConnSrv;      // Number of connected servers
  long    nbObjects;      // Number of objects
  int     docType;        // current document type
  HWND    hwndDoc;        // current document window handle
  LPVOID  lpVoidDocData;  // void pointer on extra data of the current doc
  int     cpt;
  int     max_server;

  char    bufSrv[BUFSIZE];
  char    bufNbObj[BUFSIZE];
  char    bufDate[BUFSIZE];
  char    bufTime[BUFSIZE];

  docType = QueryCurDocType(hwndFrame, &hwndDoc);
  if (docType != TYPEDOC_UNKNOWN && docType != TYPEDOC_NONE) {
    // Emb 17/07/97: GetWindowLong on null hwnd fails under unix
    HWND hDocHandle = GetVdbaDocumentHandle(hwndDoc);
    if(hDocHandle)
      lpVoidDocData = (LPVOID)GetWindowLong(hDocHandle, 0);
    else
      lpVoidDocData = NULL;
  }
  else
    lpVoidDocData = NULL;

  bGlobalStatus = TRUE;   // Was managed by timer, which has been exended a lot
  if (bGlobalStatus) {
    // 1) Date
    // TO BE FINISHED : must take windows sLongDate in account
    // at this time, takes windows sShortDate in account
    if (!NanGetWindowsDate((LPSTR)buf))
      buf[0] = '\0';
    x_strcpy(bufDate, buf);

    // 2) Time
#ifdef MAINWIN
    buf[0] = '\0';
#else
    if (!NanGetWindowsTime((LPSTR)buf))
      buf[0] = '\0';
#endif

    x_strcpy(bufTime, buf);

    // 4) Connected servers
    max_server = GetMaxVirtNodes();
    nbConnSrv = 0;
    for (cpt=0; cpt<max_server; cpt++)
      if (isVirtNodeEntryBusy(cpt))
        nbConnSrv++;


    // 6) Current server name
    switch (docType) {
      case TYPEDOC_DOM:
        lstrcpy(buf, GetVirtNodeName(
                ((LPDOMDATA)lpVoidDocData)->psDomNode->nodeHandle));
    break;
      default:
        buf[0] = '\0';
        break;
    }
    x_strcpy(bufSrv, buf);


    // 8) number of objects (don't force)
    if (docType == TYPEDOC_DOM) {
      nbObjects = GetNumberOfObjects((LPDOMDATA)lpVoidDocData, FALSE);
      if (nbObjects >= 0)
        wsprintf(buf, "%ld", nbObjects);
      else
        LoadString(hResource, IDS_GS_T_UNKNOWNOBJCOUNT, buf, sizeof(buf));
    }
    else if (docType == TYPEDOC_NONE)
      LoadString(hResource, IDS_GS_T_UNKNOWNOBJCOUNT, buf, sizeof(buf));
    else if (docType == TYPEDOC_MONITOR) {
      nbObjects = MfcGetNumberOfIpmObjects(hwndDoc);
      if (nbObjects >= 0)
        wsprintf(buf, "%ld", nbObjects);
      else
        LoadString(hResource, IDS_GS_T_UNKNOWNOBJCOUNT, buf, sizeof(buf));
    }
    else
      LoadString(hResource, IDS_GS_T_OBJECTS_NOAPPLY, buf, sizeof(buf));
    x_strcpy(bufNbObj, buf);


  MainFrmUpdateStatusBar(nbConnSrv, bufSrv, bufNbObj, bufDate, bufTime);
  }
}

//----    TO BE UNMASQUED AND FINISHED IF PERFORMANCE PROBLEMS
//----    WHEN UPDATING DATE AND TIME IN THE GLOBAL STATUS BAR
//----
//----  // variables for the date/time format management
//----  typedef struct _tagDateTime {
//----    .
//----  } INTLDATETIME, FAR *INTLDATETIME;
//----
//----  //
//----  //  Loads from the win.ini file the formats for date and time
//----  //
//----  static VOID NEAR LoadWininiDateTimeFmt()
//----  {
//----    static char section[] = "intl";
//----
//----    GetProfileString(section, "iTime"      ,"1"    , sdbInter.iTime, INFOUNIQUE);
//----    GetProfileString(section, "sTime"      ,":"    , sdbInter.sTime, INFOUNIQUE);
//----    GetProfileString(section, "s1159"      ,"A.M." , sdbInter.iTime, INFOUNIQUE);
//----    GetProfileString(section, "s2359"      ,"P.M." , sdbInter.iTime, INFOUNIQUE);
//----    GetProfileString(section, "itlZero"    ,"1"    , sdbInter.iTime, INFOUNIQUE);
//----
//----    GetProfileString(section, "sShortDate" ,"dd/MM/yy"                 , sdbInter.sShortDate,DATECOURTE);
//----    GetProfileString(section, "sLongDate"  ,"ddd', 'MMMM' 'dd', 'yyyy" , sdbInter.sLongDate,DATELONGUE);
//----
//----

//--------------------------------------------------------------------------
//
// Preferences functions
//
//--------------------------------------------------------------------------

#include "catospin.h"             // spin control
#include "subedit.h"   // subclass edits for numeric

#define TUPLE_MIN               1
#define TUPLE_MAX               9999

// Structure describing the spinning edit control
static EDITMINMAX limits[] =
{
   IDD_SQLPREF_NUM, IDD_SQLPREF_NUMSPIN, TUPLE_MIN, TUPLE_MAX,
   -1// terminator
};

//
//  Make a clone of a given font
//
static HFONT near DuplicateFont(HFONT hFont)
{
  LOGFONT         lf;

  if (!IsGDIObject(hFont))
    return (HFONT)0;

  GetObject(hFont, sizeof(lf), (LPSTR) &lf);
  return CreateFontIndirect(&lf);
}


//
//  Launch the sub-dialog that allows to choose a font for the status bar
//
static VOID NEAR ChooseGlobStatusBarFont(HWND hDlg)
{
  LOGFONT         lf;
  CHOOSEFONT      cf;
  HFONT           hFont;
  LPSTATUSBARPREF lpStatusBarPref;

  lpStatusBarPref = (LPSTATUSBARPREF)HDLGGetProp(hDlg);
  hFont = lpStatusBarPref->hFont;
  memset((char*)&cf, 0, sizeof(CHOOSEFONT));
  GetObject(IsGDIObject(hFont) ? hFont
          :GetStockObject(SYSTEM_FONT),
          sizeof(lf),(LPSTR) &lf);

  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hDlg;
  cf.lpLogFont = &lf;
  cf.Flags = CF_BOTH | CF_INITTOLOGFONTSTRUCT |CF_LIMITSIZE;
  cf.nSizeMin=4;
  cf.nSizeMax=72;
  cf.nFontType = CF_BOTH;

  if (ChooseFont(&cf)) {
    // make new font
    HFONT hNewFont = CreateFontIndirect(cf.lpLogFont);
    lpStatusBarPref->hFont = hNewFont;

    // delete old font
    if (IsGDIObject(hFont))
      DeleteObject(hFont);
  }
}

//
//  Open the dialog box that manages the DOM preferences,
//  and makes the necessary updates if OK
//
// Emb 04/03/96 : use custom template for fonts,
// duplicated from IDD_CUSTOM_FONTSETUP
// for dom font choice in 32 bit mode with Win95 tree control
//
extern void UpdateFontDOMRightPane (HFONT hFont);

BOOL ManageDomPreferences(HWND hwndMain, HWND hwndParent)
{
#ifdef  WIN32
  LOGFONT    lf;
  CHOOSEFONT cf;
  HFONT      hFont = hFontBrowsers;

  memset((char*)&cf, 0, sizeof(CHOOSEFONT));

  GetObject(IsGDIObject(hFont) ? hFont
      :GetStockObject(SYSTEM_FONT),
      sizeof(lf),(LPSTR) &lf);

  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hwndParent;
  cf.lpLogFont = &lf;
  cf.Flags = CF_BOTH | CF_INITTOLOGFONTSTRUCT |CF_LIMITSIZE;
  cf.nSizeMin=4;
  cf.nSizeMax=72;
  cf.nFontType = CF_BOTH;

  // Added Emb 21/6/95 for custom caption
  cf.Flags |= CF_ENABLETEMPLATE;
  cf.lpTemplateName = MAKEINTRESOURCE(IDD_DOM_FONTSETUP);
  cf.hInstance = hResource;

  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)5501/*domprefs_box in vdba.chm*/));
  if (ChooseFont(&cf))
    {
    // make new font
    HFONT hNewFont = CreateFontIndirect(cf.lpLogFont);

    // keep global handle on it
    hFontBrowsers = hNewFont;

    // delete old font
    if (IsGDIObject(hFont))
      DeleteObject(hFont);

    // Store in .ini
    if (IsSavePropertyAsDefault())
        WriteFontToIni(IDS_INISETTINGS, IDS_INIBROWSFONT, hFontBrowsers);

    // apply new setting to current application
    PostMessageToTrees(hwndMain, WM_SETFONT, (WPARAM)hFontBrowsers, 0L);
    UpdateFontDOMRightPane (hFontBrowsers);
    }
  lpHelpStack = StackObject_POP (lpHelpStack);
#else // WIN32

 
#endif  // WIN32
  return TRUE;
}


//
//  Launch the sub-dialog that allows to choose a font for doms
//
static VOID NEAR ChooseDomFont(HWND hDlg)
{
  LOGFONT         lf;
  CHOOSEFONT      cf;
  HFONT           hFont;
  LPDOMPREF       lpDomPref;

  lpDomPref = (LPDOMPREF)HDLGGetProp(hDlg);
  hFont = lpDomPref->hFont;
  memset((char*)&cf, 0, sizeof(CHOOSEFONT));
  GetObject(IsGDIObject(hFont) ? hFont
          :GetStockObject(SYSTEM_FONT),
          sizeof(lf),(LPSTR) &lf);

  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hDlg;
  cf.lpLogFont = &lf;
  cf.Flags = CF_BOTH | CF_INITTOLOGFONTSTRUCT |CF_LIMITSIZE;
  cf.nSizeMin=4;
  cf.nSizeMax=72;
  cf.nFontType = CF_BOTH;

  if (ChooseFont(&cf)) {
    // make new font
    HFONT hNewFont = CreateFontIndirect(cf.lpLogFont);
    lpDomPref->hFont = hNewFont;

    // delete old font
    if (IsGDIObject(hFont))
      DeleteObject(hFont);
  }
}


//
//    SQL Statements errors display
//

#include "dbaset.h"

void DisplayStatementsErrors(HWND hWnd)
{
  DLGPROC lpfnDlg;
  int     retval;
  static  BOOL bRunning;

  // recursivity forbidden!
  if (bRunning)
    return;

  bRunning = TRUE;
  lpfnDlg = (DLGPROC) MakeProcInstance((FARPROC)SqlErrorsDlgProc, hInst);
  retval = VdbaDialogBox(hResource,
                         MAKEINTRESOURCE(IDD_SQLERR),
                         hWnd,
                         lpfnDlg);
  FreeProcInstance ((FARPROC) lpfnDlg);
  bRunning = FALSE;
}

//
//  Dialog proc for the box that manages the DOM preferences
//

BOOL WINAPI __export SqlErrorsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
  switch (wMsg) {
    HANDLE_MSG(hDlg, WM_INITDIALOG, SqlErrOnInitDialog);
    HANDLE_MSG(hDlg, WM_COMMAND, SqlErrOnCommand);
    HANDLE_MSG(hDlg, WM_DESTROY, SqlErrOnDestroy);
  }
  return FALSE;
}

static void SqlErrOnDestroy(HWND hDlg)
{
    FreeSqlErrorPtr();
    HDLGRemoveProp(hDlg);
}

static void SqlErrFillControls(HWND hDlg, LPSQLERRORINFO lpsqlErrInfo)
{
  HWND hCtlStmt    = GetDlgItem(hDlg, IDC_SQLERR_STMT);
  HWND hCtlErrCode = GetDlgItem(hDlg, IDC_SQLERR_ERRCODE);
  HWND hCtlErrTxt  = GetDlgItem(hDlg, IDC_SQLERR_ERRTXT);
  char buf[20];
  int  len;
  LPSTR p, p2, txt;

  if (lpsqlErrInfo && lpsqlErrInfo->bActiveEntry) {
    if (lpsqlErrInfo->lpSqlStm)
      Edit_SetText(hCtlStmt, lpsqlErrInfo->lpSqlStm);
    else 
      Edit_SetText(hCtlStmt, "");
    if (lpsqlErrInfo->sqlcode) {
      wsprintf(buf, "%ld", lpsqlErrInfo->sqlcode);
      Static_SetText(hCtlErrCode, buf);
    }
    else 
      Static_SetText(hCtlErrCode, "");
    if (lpsqlErrInfo->lpSqlErrTxt) {
      len = x_strlen(lpsqlErrInfo->lpSqlErrTxt);
      p = lpsqlErrInfo->lpSqlErrTxt;
      while (p) {
        p = x_strchr(p, '\n');
        if (p) {
          len++;
          p++; /* dbcs compliant because this is a single byte char */
        }
      }
      txt = ESL_AllocMem(len+1);
      if (txt) {
        txt[0] = '\0';
        p = lpsqlErrInfo->lpSqlErrTxt;
        while (p) {
          p2 = x_strchr(p, '\n');
          if (p2) {
            *p2 = '\0';
            x_strcat(txt, p);
            x_strcat(txt, "\r\n");
            *p2 = '\n';
            p2++; /* dbcs compliant because this is a single byte char */
            p = p2;
          }
          else {
            x_strcat(txt, p);
            break;
          }
        }
        Edit_SetText(hCtlErrTxt, txt);
        ESL_FreeMem(txt);
      }
      else
        Edit_SetText(hCtlErrTxt, lpsqlErrInfo->lpSqlErrTxt);  // raw mode
    }
    else 
      Edit_SetText(hCtlErrTxt, "");
  }
  else {
    Edit_SetText(hCtlStmt, "");
    Edit_SetText(hCtlErrCode, "");
    Edit_SetText(hCtlErrTxt, "");
  }
}

static void SqlErrUpdButtonsStates(HWND hDlg, LPSQLERRORINFO lpsqlErrInfo)
{
  HWND hCtlFirst   = GetDlgItem(hDlg, IDC_SQLERR_FIRST);
  HWND hCtlNext    = GetDlgItem(hDlg, IDC_SQLERR_NEXT);
  HWND hCtlPrev    = GetDlgItem(hDlg, IDC_SQLERR_PREV);
  HWND hCtlLast    = GetDlgItem(hDlg, IDC_SQLERR_LAST);

  if (lpsqlErrInfo  && lpsqlErrInfo != GetOldestSqlError() )
    Button_Enable(hCtlFirst, TRUE);
  else
    Button_Enable(hCtlFirst, FALSE);
  Button_Enable(hCtlNext,  (BOOL)(GetNextSqlError(lpsqlErrInfo)) );
  Button_Enable(hCtlPrev,  (BOOL)(GetPrevSqlError(lpsqlErrInfo)) );
  if (lpsqlErrInfo  && lpsqlErrInfo != GetNewestSqlError() )
    Button_Enable(hCtlLast, TRUE);
  else
    Button_Enable(hCtlLast, FALSE);

  // post-manage focus lost (current focused button disabled)
  if (GetFocus() == NULL)
    SetFocus(GetDlgItem(hDlg, IDOK));
}

static BOOL SqlErrOnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
  LPSQLERRORINFO lpsqlErrInfo;

  CenterDialog(hDlg);
  // Update the linked list for SQL error management
  FillErrorStructure();
    
  lpsqlErrInfo = GetNewestSqlError();
  if (!HDLGSetProp(hDlg,(LPVOID)lpsqlErrInfo))
    EndDialog(hDlg,FALSE);

  SqlErrFillControls(hDlg, lpsqlErrInfo);
  SqlErrUpdButtonsStates(hDlg, lpsqlErrInfo);
  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)6024));

  return TRUE;
}

static void SqlErrOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
  LPSQLERRORINFO lpsqlErrInfo;

  lpsqlErrInfo = (LPSQLERRORINFO)HDLGGetProp(hDlg);

  switch (id) {
    case IDC_SQLERR_FIRST:
      lpsqlErrInfo = GetOldestSqlError();
      SqlErrFillControls(hDlg, lpsqlErrInfo);
      SqlErrUpdButtonsStates(hDlg, lpsqlErrInfo);
      break;

    case IDC_SQLERR_NEXT:
      lpsqlErrInfo = GetNextSqlError(lpsqlErrInfo);
      SqlErrFillControls(hDlg, lpsqlErrInfo);
      SqlErrUpdButtonsStates(hDlg, lpsqlErrInfo);
      break;

    case IDC_SQLERR_PREV:
      lpsqlErrInfo = GetPrevSqlError(lpsqlErrInfo);
      SqlErrFillControls(hDlg, lpsqlErrInfo);
      SqlErrUpdButtonsStates(hDlg, lpsqlErrInfo);
      break;

    case IDC_SQLERR_LAST:
      lpsqlErrInfo = GetNewestSqlError();
      SqlErrFillControls(hDlg, lpsqlErrInfo);
      SqlErrUpdButtonsStates(hDlg, lpsqlErrInfo);
      break;

    case IDOK:
    case IDCANCEL:
	  lpHelpStack = StackObject_POP (lpHelpStack);
      HDLGRemoveProp(hDlg);
      EndDialog(hDlg, FALSE);
      break;
  }

  // store it again in case it has been changed locally
  HDLGRemoveProp(hDlg);
  HDLGSetProp(hDlg,(LPVOID)lpsqlErrInfo);
}

void ManageNodesPreferences(HWND hwndParent)
{
  LOGFONT    lf;
  CHOOSEFONT cf;
  HFONT      hFont = hFontNodeTree;

  memset((char*)&cf, 0, sizeof(CHOOSEFONT));

  GetObject(IsGDIObject(hFont) ? hFont
      :GetStockObject(SYSTEM_FONT),
      sizeof(lf),(LPSTR) &lf);

  cf.lStructSize = sizeof(CHOOSEFONT);
  cf.hwndOwner = hwndParent;
  cf.lpLogFont = &lf;
  cf.Flags = CF_BOTH | CF_INITTOLOGFONTSTRUCT |CF_LIMITSIZE;
  cf.nSizeMin=4;
  cf.nSizeMax=72;
  cf.nFontType = CF_BOTH;

  // Added Emb 21/6/95 for custom caption
  cf.Flags |= CF_ENABLETEMPLATE;
  cf.lpTemplateName = MAKEINTRESOURCE(IDD_NODE_FONTSETUP);
  cf.hInstance = hResource;

  lpHelpStack = StackObject_PUSH (lpHelpStack, StackObject_INIT ((UINT)IDD_NODE_FONTSETUP));
  if (ChooseFont(&cf)) {
    // make new font
    HFONT hNewFont = CreateFontIndirect(cf.lpLogFont);

    // keep global handle on it
    hFontNodeTree = hNewFont;

    // delete old font
    if (IsGDIObject(hFont))
      DeleteObject(hFont);

	if (IsSavePropertyAsDefault())
	{
		// Store in .ini
		WriteFontToIni(IDS_INISETTINGS, IDS_ININODEFONT, hFontNodeTree);
	}

    // apply new setting to current application
    UpdateNodeFont(hFontNodeTree);

  }
  lpHelpStack = StackObject_POP (lpHelpStack);
}


BOOL InitialLoadEnvironment(LPSTR lpszCmdLine)
{
  char buftemp[200];
  BOOL bSuccess = FALSE;
  MdiSelWinEnd(hwndMDIClient);
  // add extension if none
	if (!x_strchr(lpszCmdLine, '.')) {
		char buf1[200];
		fstrncpy(buf1, ResourceString ((UINT)IDS_CFGFILES_FILTER),sizeof(buftemp)-1);
		if (buf1[0]!='*' || buf1[1]!='.' || x_strlen(buf1)>20) {
			myerror(ERR_FILE); // internal error
			return FALSE;
		}
		wsprintf(buftemp,"%s%s",(char *) lpszCmdLine,buf1+1);
		lpszCmdLine = buftemp;
	}


  bSuccess = OpenEnvironment(hwndFrame, lpszCmdLine);
  if (bSuccess) {
    SetOpenEnvSuccessFlag();
    bSaveRecommended = FALSE;
    UpdateAppTitle();
  }
  MdiSelWinInit(hwndMDIClient);
  return bSuccess;
}
