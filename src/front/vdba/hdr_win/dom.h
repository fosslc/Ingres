/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : dom.h
//    manages a dom document
**
**  History:
**   20-Jan-2000 (uk$so01)
**    (Bug 100063) Eliminate the undesired compiler's warning
**   26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
**  26-Mar-2002 (noifr01)
**   (bug 107442) removed the "force refresh" dialog box in DOM windows:
**    force refresh now refreshes all data
**  03-Feb-2004 (uk$so01)
**     SIR #106648, Split vdba into the small component ActiveX or COM:
**     Integrate Export assistant into vdba
**  23-Mar-2010 (drivi01)
**     Add bIsVectorWise to LPDOMDATA structure to track VW version.
********************************************************************/

#ifndef __DOM_INCLUDED__
#define __DOM_INCLUDED__

//
// Constants define
//
#define   DOM_CREATE_QUERYNODE  1   // regular dom document creation
#define   DOM_CREATE_NEWWINDOW  2   // clone of the current dom doc
#define   DOM_CREATE_TEAROUT    3   // tear out from the current dom doc
#define   DOM_CREATE_LOAD       4   // load previously saved environment
#define   DOM_CREATE_REPOSITION 5   // restart from position
#define   DOM_CREATE_SCRATCHPAD 6   // scratchpad

#define  DOM_TYPE_NORMAL        1   // Normal document
#define  DOM_TYPE_TEAROUT       2   // teared-out document
#define  DOM_TYPE_REPOS         3   // repositioned document
#define  DOM_TYPE_SCRATCHPAD    4   // scratchpad document

// TreeStatus tree bar control id's
#define RBTID_OBJTYPE      1
#define RBTID_OBJNAME      2
#define RBTID_CAPT_OWNER   3
#define RBTID_OWNERNAME    4
#define RBTID_CAPT_COMPLIM 5
#define RBTID_COMPLIM      6
#define RBTID_BSYSTEM      7

//
// Global structures definitions
//
typedef struct  _sDomNode
{
  int                     nodeHandle;
} DOMNODE, FAR * LPDOMNODE;

typedef struct  _DomData
{
  int       typeDoc;        // Must be set to TYPEDOC_DOM
  DWORD     currentRecId;   // record id for the next add item
  HWND      hwndTreeLb;
  WNDPROC   lpOldTreeProc;
  WNDPROC   lpNewTreeProc;
  HWND      hwndNanBar;
  HWND      hwndStatus;     // status bar for help text on buttons

  LPDOMNODE psDomNode;

  BOOL      bwithsystem;                  // system tables also?
  UCHAR     lpBaseOwner[MAXOBJECTNAME];   // filter on databases owner
  UCHAR     lpOtherOwner[MAXOBJECTNAME];  // filter on other objects owner

  HWND      hwndBaseOwner;                // combobox on base owner
  WNDPROC   lpOldBaseOwnerProc;           // original subclass pointer 
  WNDPROC   lpNewBaseOwnerProc;           // new subclass pointer

  HWND      hwndOtherOwner;               // combobox on other object owner
  WNDPROC   lpOldOtherOwnerProc;          // original subclass pointer 
  WNDPROC   lpNewOtherOwnerProc;          // new subclass pointer

  HWND      hwndSystem;                   // checkbox "with system"
  WNDPROC   lpOldSystemProc;              // original subclass pointer 
  WNDPROC   lpNewSystemProc;              // new subclass pointer

  // Complimentary data for the Tree Status line
  BOOL      bTreeStatus;      // TRUE if TreeStatus in use, FALSE if help text
  HWND      hwndTreeStatus;	  // Handle to the TreeStatus bar window
  HFONT     hTreeStatusFont;  // Font used for the TreeStatus bar
  BOOL      bInitTreeStatusFieldSizes;  // Must the fields be sized?

  BOOL      bShowNanBar;  // Is NanBar visible?
  BOOL      bShowStatus;  // Is status visible?

  BOOL      bCreatePhase;   // are we in create phase (not to update buttons)

  // Custom caption
  int       domType;    // one of these values exclusively:
                        // DOM_TYPE_NORMAL, DOM_TYPE_TEAROUT, DOM_TYPE_REPOS
  int       domNumber;  // sequential number used in the caption

  // notify message value for the nanbar (returned by CreateNanBarWnd() )
  UINT      uNanNotifyMsg;

  // tree lines numbers resequence
  BOOL      bMustResequence;    // must resequence
  int       delayResequence;    // cannot resequence at this time yet
                                // recursivity ---> to be tested against 0

  // for multi-platform connection
  int       ingresVer;          // version number for ingres - OIVERS_xx in dbaset.h
  BOOL	    bIsVectorWise;	// is this a VectorWise installation
  // Emb 28/05/97 for replicator : moved from static variables in domsplit.c
  int     iReplicVersion;          // 26/2/97 : added cache for GetReplicInstallStatus
  DWORD   lastDwCurSel;
  BOOL    bFirstTimeOnThisCurSel;  // 26/2/97 : added cache for ReplicatorLocalDbInstalled() calls optimization
  BOOL    bMustRequestReplicator;   // 3/09/98: instantiate - initialized to FALSE
  int     LocalDbInstalledStatus;   // 3/09/98: instantiate - initialized to RES_ERR (0)

  // Emb March 9, 98 for right click optimizations
  DWORD dwSelBeforeRClick;      // store selection "before RClick"
  int   detailSelBeforeRClick;  // store detail active Property page "before RClick"

} DOMDATA, FAR * LPDOMDATA;

//
// File Save/Load structures
//

// subset of DOMDATA, with node id replaced by node name
typedef struct  _DomDataInfo
{
  int       nodeHandle;
  DWORD     currentRecId;                 // record id for the next add item
  BOOL      bTreeStatus;                  // TRUE if TreeStatus in use
  BOOL      bwithsystem;                  // system tables also?
  UCHAR     lpBaseOwner[MAXOBJECTNAME];   // filter on databases owner
  UCHAR     lpOtherOwner[MAXOBJECTNAME];  // filter on other objects owner
  int       domType;                      // type of dom, for caption
  int       domNumber;                    // sequential number, for caption
} DOMDATAINFO, FAR * LPDOMDATAINFO;

// Complimentary informations on the tree
typedef struct _tagComplimTree {
  long  count_all;    // Total number of lines in the tree
  DWORD curRecId;     // Current selection
  DWORD FirstVis;     // Top item on the client area
} COMPLIMTREE, FAR *LPCOMPLIMTREE;

// Complimentary informations for each tree line
typedef struct _tagComplimTreeLine {
  char  caption[BUFSIZE];
  DWORD recId;
  DWORD parentRecId;
  int   recState;     // TRUE if expanded, FALSE if collapsed, -1 if no exist
} COMPLIMTREELINE, FAR *LPCOMPLIMTREELINE;

// global variables
extern char *szDomMDIChild;   // Class name for dom MDI window

extern int seqDomNormal;      // normal dom sequential number
extern int seqDomTearOut;     // teared out dom sequential number
extern int seqDomRepos;       // repositioned dom sequential number
extern int seqDomScratchpad;  // scratchpad dom sequential number

// functions in dom.c
extern BOOL DomRegisterClass(void);
extern LONG FAR PASCAL __export DomMDIChildWndProc (HWND,UINT,WPARAM,LPARAM);
extern VOID DomUpdateMenu(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bActivated);
extern BOOL CopyDomObjectToClipboard(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bQueryInfo);
extern BOOL PasteDomObjectFromClipboard(HWND hwndMdi, LPDOMDATA lpDomData, BOOL bFree);
extern void ClearDomObjectFromClipboard(HWND hwnd);
extern BOOL QueryVirtNodeName(LPSTR pBufNodeName);
extern BOOL ResequenceTree(HWND hwndMdi, LPDOMDATA lpDomData);

// functions in dom2.c (splitted from dom.c Feb 7.94)
extern VOID DomAddObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);
extern VOID DomAlterObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);
extern BOOL DomDropObject(HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);

extern VOID DomGrantObject (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);
extern VOID DomRevokeObject (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);

extern BOOL GetCurSelObjParents(LPDOMDATA lpDomData, LPUCHAR parentstrings[MAXPLEVEL]);
extern long GetCurSelComplimLong(LPDOMDATA lpDomData);

extern VOID DomModifyObjectStruct (HWND hwndMdi, LPDOMDATA lpDomData, WPARAM wParam, LPARAM lParam);

// functions in dom3.c (Properties functions)
extern VOID DomShowObjectProps(HWND hwndMdi, LPDOMDATA lpDomData);

// remote command dialog procs
BOOL WINAPI __export ExecRemoteCmdDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI __export RemotePilotDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);

// splitted : print functions
VOID PrintDom(HWND hwndMdi, LPDOMDATA lpDomData);

// splitted: remote commands fuctions
BOOL SysmodOperation(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL AlterDB(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL Populate(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL Export(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL Audit(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL GenStat(HWND hwndMdi, LPDOMDATA lpDomData);

BOOL VerifyDB(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL Checkpoint(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL RollForward(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL DisplayStat(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL UnloadDBOperation(HWND hwndMdi, LPDOMDATA lpDomData);
BOOL CopyDBOperation(HWND hwndMdi, LPDOMDATA lpDomData);

HWND CreateTreeStatusBarWnd(HWND hwndMdi);

int ManageExpandBranch(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);
int ManageExpandBranch_MT(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);

int ManageExpandAll(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);
int ManageExpandAll_MT(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);

int ManageForceRefreshCursub(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);
int ManageForceRefreshCursub_MT(HWND hwndMdi, LPDOMDATA lpDomData, DWORD dwItem, HWND hWndTaskDlg);

int ManageForceRefreshAll(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);
int ManageForceRefreshAll_MT(HWND hwndMdi, LPDOMDATA lpDomData, HWND hWndTaskDlg);

BOOL AddParentBranch(LPDOMDATA lpDomData, DWORD recId);

VOID DomRegisterAsLink(HWND hwndCurDom, LPDOMDATA lpCurDomData, BOOL bDragDrop, HWND hwndDestDom, LPDOMDATA lpDestDomData, DWORD ItemUnderMouse);
VOID DomRefreshLink(HWND hwndCurDom, LPDOMDATA lpCurDomData, BOOL bDragDrop, HWND hwndDestDom, LPDOMDATA lpDestDomData, DWORD ItemUnderMouse);

#include "tree.h"
extern BOOL CopyRPaneObjectToClipboard(HWND hwndMdi, int rPaneObjType, BOOL bQueryInfo, LPTREERECORD lpRecord);

#endif //__DOM_INCLUDED__
