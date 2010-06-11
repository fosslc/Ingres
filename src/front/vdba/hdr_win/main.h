/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : main.h
//    creates the application and the frame window
//    manages the WM_COMMAND messages
//
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  05-Jul-2001 (hanje04)
**	Added defn of A2W for MAINWIN as the MAINWIN defn differs from
**	the MS VC6 defn.
**  19-Oct-2001 (schph01)
**    bug 106086 add prototype of GetVdbaHelpStatus () function.
**  28-Mar-2002 (noifr01)
**    (sir 99596) removed additional unused resources/sources
**  02-Jun-2010 (drivi01)
**    Increase the size of BUFSIZE and use DB_MAXNAME to define it
**    instead of hardcoded value.  It looks like DB_MAXNAME
**    is used to store table names and other objects, i believe
**    4 times the object size should be big enough and consistent
**    with what was there before.
********************************************************************/

#ifndef __MAIN_INCLUDED__
#define __MAIN_INCLUDED__
#include "port.h"
#include "dba.h"
//
// constants definition
//
#define BUFSIZE     MAXOBJECTNAME*4 // size of buffers
#define WINDOWMENU  6     // position of window menu in the menu bar

// Document type
#define TYPEDOC_UNKNOWN   -1
#define TYPEDOC_NONE      0
#define TYPEDOC_DOM       1       // DOM doc
#define TYPEDOC_SQLACT    2       // SQL Activity document
#define TYPEDOC_PROPERTY  3       // Object property document
// new mfc documents for version 2
#define TYPEDOC_MONITOR   4       // Monitor window   
#define TYPEDOC_DBEVENT   5       // Database events trace window
#define TYPEDOC_NODESEL   6       // Nodes selection in non-docking view mode

// custom messages
#define WM_USERMSG_FIND             (WM_USER + 1)
#define WM_SETUPMESSAGE             (WM_USER + 2)
#define WM_USER_CHANGE_PREFERENCES  (WM_USER + 3) // wParam and lParam to 0
#define WM_NOTIFY_SQLCONNECT        (WM_USER + 4) // SqlAct: database changed
#define WM_F1DOWN                   (WM_USER + 5) // Intercepting F1 to handle Help
#define WM_USER_GETNODEHANDLE       (WM_USER + 6) // Get node handle from mfc documents
#define WM_USER_GETMFCDOCTYPE       (WM_USER + 7) // Get document type from mfc documents

//
// Structures
//

// Save/Load structure, common to every mdi document type
// NOTE : DOCINFO is already defined in windows.h...
typedef struct _tagDocInfo {
  char  szTitle[BUFSIZE];   // mdi doc window caption
  int   x;                  // horizontal position
  int   y;                  // vertical position
  int   cx;                 // width
  int   cy;                 // height
  BOOL  bIcon;              // TRUE if document in icon state
  BOOL  bZoom;              // TRUE if document maximized
  int   ingresVer;          // version number for ingres - OIVERS_xx in dbaset.h
} COMMONINFO, FAR *LPCOMMONINFO;

// External variables declaration
extern HINSTANCE  hInst;            // application instance handle
extern HINSTANCE  hResource;        // external .dll handle for resource data
extern HWND       hwndFrame;        // main window handle
extern HWND       hwndMDIClient;    // handle of MDI Client window
extern HWND       hwndToolBar;      // Handle to global tool bar
extern HWND       hwndStatusBar;    // Handle to global status bar
extern BOOL       bGlobalStatus;    // TRUE if GlobalStatus in use, FALSE if help text
extern HWND       hwndGlobalStatus; // Handle to the GlobalStatus bar window
extern BOOL       bSaveCompressed;  // Currently a global indicating whether to save files
                                    // as compressed files
extern BOOL       bReadCompressed;

// fonts
extern HFONT  hFontStatus;          // Font used for all status bars
extern HFONT  hFontBrowsers;        // Font used for all browsers
extern HFONT  hFontPropContainers;  // Font for all properties containers

extern HFONT  hFontIpmTree;         // Font for all monitor trees
extern HFONT  hFontNodeTree;        // Font for the node tree

extern LONG   styleDefault;     // default child creation state
extern char   szChild[];        // class of child

// modeless dialog boxes
extern HWND   hDlgRefresh;      // refresh dialog box
extern HWND   hDlgErrorsTrack;  // Error track dialog box

// environment 
extern BOOL bSaveRecommended;   // dirty environment?

// Clipboard management
extern WORD wDomClipboardFormat;
extern BOOL bDomClipboardDataStored;  // data stored with dom format
extern int  DomClipboardObjType;      // type of last object copied in clipbd

// 'Find' item
extern HWND       hDlgSearch;       // handle on "find" modeless


// Multi-menu management added July 26, 96
extern HMENU hGwNoneMenu;           // when doc on OpenIngres node (no gateway)

// Dialog boxes parenthood management
BOOL DialogInProgress(void);
HWND GetCurrentDialog(void);

//
// External functions
//
extern HWND FAR PASCAL MakeNewChild          (char *);
extern LONG FAR PASCAL __export FrameWndProc (HWND,UINT,WPARAM,LPARAM);
extern int QueryDocType(HWND hwndDoc);

// Settings change reflect functions
extern BOOL QueryShowNanBarSetting(int typedoc);
extern BOOL QueryShowStatusSetting(int typedoc);

// special function for reentrance problems versus IIPROMPTALL
void StoreMessage(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

// Timers ids
#define IDT_BKTASK          1   // background task - FrameWndProc
#define IDT_GLOBALSTATUS    2   // global status bar update - FrameWndProc
#define IDT_REMOTECMD       3   // Remote command - ExecRemoteCmdDlgProc
#define IDT_DOMSTATUS       4   // doms status bars update - FrameWndProc

// For background refresh displaying a modeless dialog
// while the application is in the back
extern BOOL IsTheActiveApp();
extern HWND GetStoredFocusWindow();

void PushVDBADlg(void);
void PopVDBADlg(void);

// Added Emb 25/06/97 to prevent disable of application
// if expanding comboboxes in toolbars calling domgetfirst()
void SetExpandingCombobox();
void ResetExpandingCombobox();

// update of global status managed by OnIdle() in mainmfc.cpp
void UpdateGlobalStatus(void);

HWND GetVdbaDocumentHandle(HWND hwndDoc);

#ifdef MAINWIN
#define A2W(a, w, cb)	MultiByteToWideChar(			\
						CP_ACP,		\
						0,		\
						a,		\
						-1,		\
						w,		\
						cb)
#endif

BOOL GetVdbaHelpStatus (void);

#endif //__MAIN_INCLUDED__
