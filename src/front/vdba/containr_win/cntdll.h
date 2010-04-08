/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTDLL.H
 *
 * DLL Specific include file.
 *
 * Contains all definitions and and prototypes pertinent ONLY
 * to the DLL.  API related information is contained in CATOCNTR.H.
 *
 * History:
 *	16-feb-2000 (somsa01)
 *	    Generisize include of vdba headers.
 *  02-Jan-2002 (noifr01)
 *      (SIR 99596) removed usage of unused DLLs
 ****************************************************************************/

#include <custcntl.h>   // Windows header file for custom controls.
#include "ll.h"         // Get ADT interface for the linked list.
#include "catocntr.h"   // Get interface stuff for the control.


HINSTANCE hInstCnt;               // Container instance handle

// This macro extracts the Container struct pointer from the wnd handle.
#define GETCNT(hw) ((LPCONTAINER) GetWindowLong((hw), GWL_CONTAINERHMEM ))

#define CNT_DEFWNDPROC  DefWindowProc    // default proc for main cnt wndproc
#define CNT_DEFDLGPROC  CntDefDlgProc    // default proc for cnt dlgproc

#define ASYNC_KEY_DOWN  0x8000   // flag indicating key is pressed

#define CTRL_X          0x0018   // keyboard equivalent of WM_CUT
#define CTRL_C          0x0003   // keyboard equivalent of WM_COPY
#define CTRL_V          0x0016   // keyboard equivalent of WM_PASTE
#define CTRL_R          0x0012   // keyboard equivalent of WM_CLEAR
#define CTRL_Z          0x001A   // keyboard equivalent of WM_UNDO

/*--------------------------------------------------------------------------*
 * Container version number
 *--------------------------------------------------------------------------*/
#define CNT_VERSION     0x0202

/*--------------------------------------------------------------------------*
 * Split bar structure
 *--------------------------------------------------------------------------*/
typedef struct tagSPLIT
    {
    int   xSplitStr;      // Xcoord of the split bar start
    int   xSplitEnd;      // Xcoord of the split bar end
    float fSplitPerCent;  // xSplitStr as a percentage of frame client width
    HWND  hWndLeft;       // child window on left  side of the split bar
    HWND  hWndRight;      // child window on right side of the split bar
    } SPLIT;

typedef SPLIT      *PSPLIT;
typedef SPLIT FAR *LPSPLIT;

#define LEN_SPLIT sizeof(SPLIT)

/*--------------------------------------------------------------------------*
 * Container Notification Event structure (used with CTS_ASYNCNOTIFY style)
 *--------------------------------------------------------------------------*/

typedef struct tagCNEVENT
    {
    HWND     hWndCurCNSender;  // child wnd sending the current notification
    UINT     wCurNotify;       // current notification value
    UINT     wCurAnsiChar;     // current ansi char value (for CN_CHARHIT)
    LONG     lCurCNInc;        // current scrolling increment
    BOOL     bCurCNShiftKey;   // state of the shift key for current notify
    BOOL     bCurCNCtrlKey;    // state of the control key for current notify
    LPRECORDCORE lpCurCNRec;   // current notification record
    LPFIELDINFO  lpCurCNFld;   // current notification field
    LPVOID   lpCurCNUser;      // current notification user data
    } CNEVENT;

typedef CNEVENT      *PCNEVENT;
typedef CNEVENT FAR *LPCNEVENT;

#define LEN_CNEVENT sizeof(CNEVENT)

/*--------------------------------------------------------------------------*
 * Flowed view column info
 *--------------------------------------------------------------------------*/
typedef struct tagFLOWCOLINFO
{
    struct tagFLOWCOLINFO FAR *lpNext;   // pointer to next field
    struct tagFLOWCOLINFO FAR *lpPrev;   // pointer to previous field
    UINT     cxWidth;                    // display width of the field in characters
    UINT     cxPxlWidth;                 // display width of the field in pixels
    int      nRecs;                      // number of records in the column
    LPRECORDCORE lpTopRec;               // Top Record in the column
} FLOWCOLINFO;

typedef FLOWCOLINFO      *PFLOWCOLINFO;
typedef FLOWCOLINFO FAR *LPFLOWCOLINFO;

#define LEN_FLOWCOLINFO sizeof(FLOWCOLINFO)

/*--------------------------------------------------------------------------*
 * Draginprogress info struct
 *--------------------------------------------------------------------------*/
typedef struct tagDRGINPROG
{
    struct tagDRGINPROG FAR *lpNext;   // pointer to next field
    int           nIndex;              // place holder index
    char          szKey[20];           // unique shared memory name
    LPCNTDRAGINIT lpDragInit;          // pointer to the draginit structure for this drag operation
    LPCDRAGINFO   lpDragInfo;          // pointer to the draginfo structure for this drag operation
    HANDLE        hMapFileHandle;      // handle to disk file used for storage
    HANDLE        hDragMem;            // handle to shared memory
    LPVOID        lpDragView;          // pointer to shared memory

    
}DRGINPROG;

typedef DRGINPROG       *PFDRGINPROG;
typedef DRGINPROG   FAR *LPDRGINPROG;

#define LEN_CNTDRGINPROG sizeof(DRGINPROG)


typedef struct tagDROPINPROG
{
  struct tagDROPINPROG FAR *lpNext;     // pointer to next field
  int                       nIndex;      // place holder index
  char                      szKey[20];   // unique shared memory name
  LPCDRAGINFO               lpDragInfo;  // pointer to the draginfo structure for this drop operation
  HANDLE                    hDragHandle; // handle to shared memory

}DROPINPROG;

typedef DROPINPROG       *PFDROPINPROG;
typedef DROPINPROG   FAR *LPDROPINPROG;

#define LEN_CNTDROPINPROG sizeof(DROPINPROG)


typedef struct tagRENDERDATA
{
  struct tagRENDERDATA *lpNext;               // pointer to render next render index struct
  int                  nRenderIndex;          // id of this render struct
  HANDLE               hDragHandle;           // handle to shared memory
  char                 szKey[20];             // unique shared memory name
  LPCNTDRAGTRANSFER    lpDragTransfer;        // pointer to dragtransfer struct during render data message

}RENDERDATA;

typedef RENDERDATA        *PFRENDERDATA;
typedef RENDERDATA   FAR  *LPRENDERDATA;

#define LEN_CNTRENDERDATA  sizeof(RENDERDATA)


/*--------------------------------------------------------------------------*
 * In window extra bytes we store a handle to a CONTAINER data structure.
 *--------------------------------------------------------------------------*/

typedef struct tagCONTAINER
    {
    HWND      hWndAssociate;    // Associate window handle
    LPCNTINFO lpCI;             // pointer to container info struct
    DWORD     dwStyle;          // Copy of GetWindowLong(hWnd, GWL_STYLE)
    UINT      iView;            // Current view
    int       iMin;             // Min of vert range (usually 0)
    int       iMax;             // Max of vert range (total records possible)
    int       cxClient;         // width of client area in pixels
    int       cyClient;         // height of client area in pixels
    int       cxWndOrig;        // original width of container (including NC area)
    int       cyWndOrig;        // original height of container (including NC area)
    int       iPos;             // Current position
    int       nVscrollMin;      // minimum of vert scroll range
    int       nVscrollMax;      // maximum of vert scroll range
    int       nVscrollPos;      // current position of vert scroll
    int       nHscrollMin;      // minimum of horz scroll range
    int       nHscrollMax;      // maximum of horz scroll range
    int       nHscrollPos;      // current position of horz scroll
    int       nDefaultFont;     // default font for GetStockObject (usually ANSI_VAR_FONT)
    UINT      wState;           // State flags
    RECT      rectPressedBtn;   // rect of currently pressed title or field button
    LPFIELDINFO  lpPressedFld;  // ptr to currently pressed field button
    LPFIELDINFO  lpSizMovFld;   // ptr to field being sized or moved
    LPFIELDINFO  lp1stSelFld;   // ptr to 1st field in current selection
    LPRECORDCORE lp1stSelRec;   // ptr to 1st record in current selection
    RECT      rectCursorFld;    // rect of field of current cursor position
    RECT      rectRecArea;      // rect of record area 
    int       yLastRec;         // bottom yCoord of last visible record
    int       nCurRec;          // visible record cursor is over (used during selection)
    int       xStartPos;        // starting x coord for field sizing/moving
    int       yStartPos;        // starting y coord for field sizing/moving
    int       xPrevPos;         // previous x coord for field sizing/moving
    int       yPrevPos;         // previous y coord for field sizing/moving
    int       cxMoveFld;        // width of rect for field moving
    int       cyMoveFld;        // height of rect for field moving
    RECT      rectMoveFld;      // rect of currently moving field
    int       xOffMove;         // x offset from mouse point to moving rect org
    int       yOffMove;         // y offset from mouse point to moving rect org
    int       xLastCol;         // right border of the last column
    int       nNestedDeferPaint; // counter for nested CntDeferPaint calls
    BOOL      bMoving;          // flag for moving fields
    BOOL      bMovingFld;       // flag for moving fields used by integral size code
    BOOL      bIntegralSizeChg; // flag for self size chg (if TRUE cxWndOrig & cyWndOrig are not changed in WM_SIZE)
    BOOL      bRetainBaseHt;    // user flag for keeping base height (if TRUE cyWndOrig is not changed in WM_SIZE)
    HWND      hWndFrame;        // hwnd to container frame wnd
    HWND      hWnd1stChild;     // hwnd to permanent rightmost child
    HWND      hWndLastFocus;    // hwnd to the child which had focus last
    BOOL      bIsSplit;         // flag indicating split or not
    BOOL      bSelectList;      // flag indicating selected records or fields
    BOOL      bSimulateShftKey; // flag indicating we are simulating a shift key state
    BOOL      bSimulateCtrlKey; // flag indicating we are simulating a control key state
    BOOL      bSimulateMulSel;  // flag indicating we are simulating multiple select while in extended select
    BOOL      bWideFocusRect;   // flag indicating the app has a wide focus rect
    BOOL      bDoingQueryDelta; // flag indicating the container has issued a CN_QUERYDELTA msg
    BOOL      bScrollFocusRect; // flag for controlling the 'snap-back' to focus behavior
    SPLIT     SplitBar;         // The one split bar.
    int       nBarWidth;        // Width of split bar
    int       nHalfBarWidth;    // Half the width of split bar 
    int       nMinWndWidth;     // minimum width of right wnd after split.
    HFONT     hFontDefault;     // handle to default general container font
    HCURSOR   hCurSplitBar;     // handle to split bar dragging cursor 
    COLORREF  crColor[CNTCOLORS];  // container colors for text drawing
    HPEN      hPen[CNTCOLORS];  // container colors for line drawing
    PAINTPROC lpfnPaintBk;      // owner function for painting background areas
    LONG      lCurCNInc;        // current scrolling increment (LONG)
    LONG      lDelta;           // scrolling limit from either end of list (LONG)
    LONG      lDeltaPos;        // starting position for delta calculation (LONG)
    int       yCursorStart;     // starting cursor position for 32 bit scrolling
    int       nVPosStart;       // starting Vscroll position for 32 bit scrolling
    int       nVPosPerPxl;      // nbr of positions per pixel for 32 bit scrolling
    LPFLOWCOLINFO lpColHead;    // pointer to beginning of column list for flowed view
    LPFLOWCOLINFO lpColTail;    // pointer to end of column list for flowed view
    LPRECORDCORE lpRecAnchor;   // used in extended select shift mode as first rec selected
    BOOL      bSplitSave;       // when changing views from detail, saves if window was split or not.
    int       xSplitSave;       // when changing views from detail, coordinate of splitbar
    int       iLeftScrollPos;   // when changing views from detail, scroll pos of left window
    int       iRightScrollPos;  // when changing views from detail, scroll pos of right window
    LPCDRAGINFO   lpTargetInfo; // pointer to the drag info structure received on wm_cntdragover
    LPRECORDCORE  lpDragOverRec;   // ptr to record where dragover is occuring
    LPRECORDCORE  lpDropOverRec;   // ptr to record where drop occurred.
    LPCNTDRAGTRANSFER lpDragTransfer;  // ptr to the dragtransfer info struct received on wm_renderdata
    LPCNTDRAGTRANSFER lpRenderComplete; // ptr to the dragtransfer info for item that was rendered
    UINT          uWmDragOver;     // id of the wm_cntdragover message
    UINT          uWmDrop;         // id of the wm_cntdrop message
    UINT          uWmDragLeave;    // id of the wm_cntdragleave message
    UINT          uWmDragComplete; // id of the wm_cntdragleave message
    UINT          uWmRenderData;   // id of the wm_cntrenderdata message
    UINT          uWmRenderComplete;  // id of the wm_cntrendercomplete message
    int           nDrgop;          // index number for each drag operation lpdrginprog struct 
    int           nDropop;         // index number ofr each drop operation lpdropinprog struct
    char          szDropKey[20];   // key to current drop operation
    DWORD         dwPid;            // our process ID
    int           nDropComplete;   // index number to lpdrginprog that has completed
    int           nRenderIndex;
    LPDRGINPROG   lpDrgInProg;     // link list of drag operations in progress
    LPDROPINPROG  lpDropInProg;    // link list of drop operations in progress
    LPRENDERDATA  lpRenderData;    // data structure used during render operation in progress
    LRESULT       lReturnMsgVal;   // 32-bit value returned from certain notification msgs.
    int           iInputID;        // contains message id when a lbuttondown or keydown msg processed.
    } CONTAINER;

typedef CONTAINER     *PCONTAINER;
typedef CONTAINER FAR *LPCONTAINER;

#define LEN_CONTAINER sizeof(CONTAINER)

/*--------------------------------------------------------------------------*
 * Container class/window extra bytes.
 *--------------------------------------------------------------------------*/

// Number of extra bytes for the container class.
#define LEN_CLASSEXTRA       0

// Offsets to use with GetWindowLong to get at the extra bytes
#define GWL_CONTAINERHMEM    0   // handle to the container struct
#define GWL_SHOWVSCROLL      4   // flag for showing the vertical scroll bar

// Number of extra bytes for the container window.
#define LEN_WINDOWEXTRA      16

#ifdef WIN16
#define CNTFRAME_CLASS       "CA_ContainerFrame"
#define CNTCHILD_CLASS       "CA_ContainerChild"
#else
#define CNTFRAME_CLASS       "CA_ContainerFrame32"
#define CNTCHILD_CLASS       "CA_ContainerChild32"
#endif

/*--------------------------------------------------------------------------*
 * IDs for internal Container controls.
 *--------------------------------------------------------------------------*/

#define IDC_HSCROLLBAR      701
#define IDC_VSCROLLBAR      702

/*--------------------------------------------------------------------------*
 * Container state flags.
 *--------------------------------------------------------------------------*/

#define CNTSTATE_GRAYED      0x0001
#define CNTSTATE_HIDDEN      0x0002
#define CNTSTATE_MOUSEOUT    0x0004  // mouse position is outside control
#define CNTSTATE_CLICKED     0x0008  // mouse button 1 is down
#define CNTSTATE_DEFERPAINT  0x0010  // do not update while this is set
#define CNTSTATE_VSCROLL32   0x0020  // vertical range max is 32 bit (greater than 32767)
#define CNTSTATE_HASFOCUS    0x0040  // container has the focus
#define CNTSTATE_CLKEDTTLBTN 0x0080  // mouse button is down on title button
#define CNTSTATE_CLKEDFLDBTN 0x0100  // mouse button is down on field button
#define CNTSTATE_SIZINGFLD   0x0200  // mouse button is down on field size area
#define CNTSTATE_MOVINGFLD   0x0400  // mouse button is down on field title area
#define CNTSTATE_CREATINGSB  0x0800  // creating a splitbar child wnd

// Combination of state flags.
#define CNTSTATE_ALL         0x0FFF

/*--------------------------------------------------------------------------*
 * Macros to change the control state given an LPCONTAINER and state flags.
 *--------------------------------------------------------------------------*/

#define StateSet(lp, wFlags)    (lp->wState |=  (wFlags))
#define StateClear(lp, wFlags)  (lp->wState &= ~(wFlags))
#define StateTest(lp, wFlags)   (lp->wState &   (wFlags))

/*--------------------------------------------------------------------------*
 * Macros to move thru the linked lists.
 *--------------------------------------------------------------------------*/

#define NEXTFIELD(p)  ( (LPFIELDINFO)  NEXT(p) )
#define PREVFIELD(p)  ( (LPFIELDINFO)  PREV(p) )
#define NEXTRECORD(p) ( (LPRECORDCORE) NEXT(p) )
#define PREVRECORD(p) ( (LPRECORDCORE) PREV(p) )

/*--------------------------------------------------------------------------*
 * Timer identifiers.
 *--------------------------------------------------------------------------*/

#define IDT_FIRSTCLICK      500
#define IDT_HOLDCLICK       501
#define IDT_AUTOSCROLL      502

#define CTICKS_FIRSTCLICK   400
#define CTICKS_HOLDCLICK    50
#define CTICKS_AUTOSCROLL   50

/*--------------------------------------------------------------------------*
 * Default range and position constants.
 *--------------------------------------------------------------------------*/

#define IDEFAULTMIN         0
#define IDEFAULTMAX         100
#define IDEFAULTPOS         0

/*--------------------------------------------------------------------------*
 * Misc constants
 *--------------------------------------------------------------------------*/

#define MAX_STRING_LEN      256      // max len of string data per field
#define STR_LEN             50       // default string length
#define FLDSIZE_HIT         5        // width of col separator cursor hit test
#define SPLITBAR_CREATE     0x6600   // flag for creating a new split bar
#define SPLITBAR_MOVE       0x6611   // flag for moving an existing split bar
#define SPLITBAR_DELETE     0x6622   // flag for deleting a split bar
#define CLR_INIT            0        // flag for initializing colors
#define CLR_UPDATE          1        // flag for updating colors
#define CLR_DELETE          2        // flag for deleting color objects

/*--------------------------------------------------------------------------*
 * Stringtable identifiers
 *--------------------------------------------------------------------------*/

#define IDS_FULLNAME            1
#define IDS_SHORTNAME           2
#define IDS_CREDITS             3
#define IDS_RANGEERROR          4 

// Style strings for the dialog editor
#define IDS_CTS_READONLY        5 
#define IDS_CTS_SINGLESEL       6 
#define IDS_CTS_MULTIPLESEL     7
#define IDS_CTS_EXTENDEDSEL     8
#define IDS_CTS_BLOCKSEL        9
#define IDS_CTS_SPLITBAR        10
#define IDS_CTS_TABSTOP         11
#define IDS_CTS_GROUP           12
#define IDS_CTS_VISIBLE         13
#define IDS_CTS_VERTSCROLL      14
#define IDS_CTS_HORZSCROLL      15
#define IDS_CTS_HASBORDER       16
#define IDS_CTS_HASDLGFRAME     17
#define IDS_CTS_HASCAPTION      18
#define IDS_CTS_SYSMENU         19
#define IDS_CTS_INTEGRALWIDTH   20
#define IDS_CTS_INTEGRALHEIGHT  21
#define IDS_CTS_EXPANDLASTFLD   22

// String IDs for the font specifications
#define IDS_FONT_DEFAULT        23
#define IDS_FONT_FACENAME       24
#define IDS_FONT_POINTSIZE      25
#define IDS_FONT_WEIGHT         26
#define IDS_FONT_FAMILY         27
#define IDS_FONT_PITCH          28
#define IDS_FONT_CHARSET        29

// String IDs for the error messages
#define IDS_DLL_ERR            100
#define IDS_REGCLASS_ERR       101
#define IDS_INITPROC_ERR       102
#define IDS_INITHREAD_ERR      103
#define IDS_KILLPROC_ERR       104
#define IDS_KILLTHREAD_ERR     105
#define IDS_STYLEDLG_ERR       106

#define LEN_ERRMSG              60

/*--------------------------------------------------------------------------*
 * Function Prototypes
 *--------------------------------------------------------------------------*/

// CNTWEP.C
int     WINAPI WEP( int nExitType );

// CNTINIT.C
int     WINAPI LibMain( HINSTANCE, UINT, UINT, LPSTR );

// CATOCNTR.C
LRESULT WINAPI FrameWndProc( HWND, UINT, WPARAM, LPARAM );
LRESULT WINAPI ContainerWndProc( HWND, UINT, WPARAM, LPARAM );
VOID    WINAPI InitTextMetrics( HWND hWnd, LPCONTAINER lpCnt );
VOID    WINAPI AdjustVertScroll( HWND hWnd, LPCONTAINER lpCnt, BOOL bSizing );
VOID    WINAPI UpdateContainer( HWND hWnd, LPRECT lpRect, BOOL bEraseBkGrnd );
BOOL    WINAPI ContainerDestroy( HWND hWnd, LPCONTAINER lpCnt );
VOID    WINAPI NotifyAssocWnd( HWND hWnd, LPCONTAINER lpCnt, UINT wEvent, UINT wOemCharVal,
                               LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                               BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData );
LRESULT    WINAPI NotifyAssocWndEx( HWND hWnd, LPCONTAINER lpCnt, UINT wEvent, UINT wOemCharVal,
                               LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                               BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData );
BOOL    WINAPI IsFocusInView( HWND hWnd, LPCONTAINER lpCnt );
VOID    WINAPI ScrollToFocus( HWND hWnd, LPCONTAINER lpCnt, UINT wVertDir );
VOID    WINAPI CntColorManager( HWND hWnd, LPCONTAINER lpCnt, UINT wAction );
COLORREF WINAPI GetCntDefColor( UINT iColor );
VOID    CALLBACK CntTimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime );

// CVDETAIL.C
LRESULT WINAPI DetailViewWndProc( HWND, UINT, WPARAM, LPARAM );
UINT    WINAPI GetRecordData( HWND hWndCnt, LPRECORDCORE lpRec, LPFIELDINFO lpField, UINT wMaxLen, LPSTR lpStrData );
int     WINAPI InvalidateCntRecord( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                    LPVOID lpFldCol, UINT fMode );
int     WINAPI InvalidateCntRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                 LPFIELDINFO lpFld, UINT fMode );
int     WINAPI InvalidateCntRecTxt( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord,
                                    LPFLOWCOLINFO lpCol, UINT fMode );
BOOL    WINAPI GetFieldXPos( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPINT lpxStr, LPINT lpxEnd );
BOOL    WINAPI PtInTtlBtn( LPCONTAINER lpCnt, LPPOINT lpPt );
BOOL    WINAPI PtInFldTtlBtn( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt, LPINT lpStr, LPINT lpEnd );
BOOL    WINAPI PtInFldSizeArea( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt );
VOID    WINAPI ResizeCntFld( HWND hWnd, LPCONTAINER lpCnt, int xNewRight );
VOID    WINAPI ReorderCntFld( HWND hWnd, LPCONTAINER lpCnt, int xNewRight );
VOID    WINAPI DrawSizingLine( HWND hWnd, LPCONTAINER lpCnt, int xNewRight );
VOID    WINAPI InitMovingRect( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpCol, LPPOINT lpPt );
VOID    WINAPI DrawMovingRect( HWND hWnd, LPCONTAINER lpCnt, LPPOINT lpPt );
BOOL    WINAPI IsRecInView( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord );
BOOL    WINAPI IsFldInView( HWND hWnd, LPCONTAINER lpCnt, LPFIELDINFO lpFld );
int     WINAPI ClearExtSelectList( HWND hWnd, LPCONTAINER lpCnt );
VOID    WINAPI ClearBlkSelectList( HWND hWnd, LPCONTAINER lpCnt );
LPFIELDINFO WINAPI GetFieldAtPt( LPCONTAINER lpCnt, LPPOINT lpPt, LPINT lpXstr, LPINT lpXend );
LPFIELDINFO WINAPI GetFldToBeSized( LPCONTAINER lpCnt, LPFIELDINFO lpFld, LPPOINT lpPt, LPINT lpXright );
LPFIELDINFO WINAPI Get1stFldInView( HWND hWnd, LPCONTAINER lpCnt );
int     CalcVThumbpos( HWND hWnd, LPCONTAINER lpCnt );
BOOL    ExtSelectRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec,
                      int nSelRecs, LPRECORDCORE FAR *lpRecLast, BOOL bShiftKey );
int     FindFocusRec( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec );
VOID    InitVThumbpos( HWND hWnd, LPCONTAINER lpCnt );
BOOL    BatchSelect( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecStr,
                     LPRECORDCORE lpRecEnd, int nDir );
VOID    Dtl_OnShowWindow( HWND hWnd, BOOL fShow, UINT status );
VOID    Dtl_OnCancelMode( HWND hWnd );
VOID    Dtl_OnCommand( HWND hWnd, int id, HWND hwndCtl, UINT codeNotify );
VOID    Dtl_OnEnable( HWND hWnd, BOOL fEnable );
VOID    Dtl_OnChildActivate( HWND hWnd );
VOID    Dtl_OnDestroy( HWND hWnd );
UINT    Dtl_OnGetDlgCode( HWND hWnd, LPMSG lpmsg );

// CNTSPLIT.C
BOOL    WINAPI SplitBarManager( HWND hWndFrame, LPCONTAINER lpCnt, UINT wEvent, LPPOINT lpPt );
HWND    WINAPI CreateSplitBar( HWND hWndFrame, LPCONTAINER lpCnt, int xSplit );
BOOL    WINAPI DragSplitBar( HWND hWnd, LPCONTAINER lpCnt, LPRECT lpRect, LPPOINT lpPtStart, LPPOINT lpPtEnd );
VOID    WINAPI MoveSplitBar( LPCONTAINER lpCnt, int xOldSplit, int xMovSplit );
VOID    WINAPI DeleteSplitBar( LPCONTAINER lpCnt, int xSplit );
VOID    WINAPI GetChildPos( HWND hWnd, LPINT lpXorg, LPINT lpYorg, LPINT lpWidth, LPINT lpHeight );
HWND    WINAPI GetChildAtPt( HWND hWndFrame, LPCONTAINER lpCnt, int xCoord );
LPSPLIT WINAPI GetSplitBar( LPCONTAINER lpCnt, int xCoord );

// DTLPAINT.C
VOID    WINAPI PaintDetailView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs );

// CVICON.C
LRESULT WINAPI IconViewWndProc( HWND, UINT, WPARAM, LPARAM );
LPZORDERREC    CntNewOrderRec( void );
BOOL           CntFreeOrderRec( LPZORDERREC lpOrderRec );
BOOL           CntAddRecOrderHead( HWND hCntWnd, LPZORDERREC lpNew );
BOOL           CntAddRecOrderTail( HWND hCntWnd, LPZORDERREC lpNew );
BOOL           CntKillRecOrderLst( HWND hCntWnd );
LPZORDERREC    CntFindOrderRec( HWND hCntWnd, LPRECORDCORE lpRec );
LPZORDERREC    CntRemoveOrderRec(HWND hCntWnd, LPZORDERREC lpOrderRec);
BOOL           CreateRecOrderList( HWND hCntWnd );
BOOL           CreateIconTextRect( HDC hDC, LPCONTAINER lpCnt, POINT ptIconOrg, LPSTR szText, RECT *lpRect );
BOOL           CalculateIconWorkspace( HWND hWnd );
VOID           IcnAdjustHorzScroll( HWND hWnd, LPCONTAINER lpCnt );
int     WINAPI InvalidateCntRecIcon( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRecord, UINT fMode );
BOOL           AdjustIconWorkspace( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpRec );
LPZORDERREC    FindClickedRecordIcn( HWND hWnd, LPCONTAINER lpCnt, POINT pt );
LPRECORDCORE   FindNextRecIcn( HWND hWnd, LPCONTAINER lpCnt, LPRECORDCORE lpFocusRec, UINT vkCode );
BOOL WINAPI    IcnIsFocusInView( HWND hWnd, LPCONTAINER lpCnt );
VOID WINAPI    IcnScrollToFocus( HWND hWnd, LPCONTAINER lpCnt );

// ICNPAINT.C
VOID    WINAPI PaintIconView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs );
BOOL           DrawBitmapXY(LPCONTAINER lpCnt, LPRECORDCORE lpRec, HDC hDC, int nXstart, 
                            int nYstart, LPRECT lpImageRect);
BOOL           DrawIconXY(LPCONTAINER lpCnt, LPRECORDCORE lpRec, HDC hDC, int nXstart, 
                          int nYstart, int nXwidth, int nYheight);

// CVNAME.C
LRESULT WINAPI NameViewWndProc( HWND, UINT, WPARAM, LPARAM );

// NAMPAINT.C
VOID    WINAPI PaintNameView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs );

// CVTEXT.C
LRESULT WINAPI TextViewWndProc( HWND, UINT, WPARAM, LPARAM );
BOOL CreateFlowColStruct( HWND, LPCONTAINER );
BOOL WINAPI TxtIsFocusInView( HWND hWnd, LPCONTAINER lpCnt );
VOID WINAPI TxtScrollToFocus( HWND hWnd, LPCONTAINER lpCnt, UINT wVertDir );
BOOL Txt_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Txt_OnSize( HWND hwnd, UINT state, int cx, int cy );
VOID Txt_OnSetFont( HWND hWnd, HFONT hfont, BOOL fRedraw );
VOID Txt_OnSetFocus( HWND hWnd, HWND hwndOldFocus );
VOID Txt_OnKillFocus( HWND hWnd, HWND hwndNewFocus );
VOID Txt_OnVScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Txt_OnHScroll( HWND hWnd, HWND hwndCtl, UINT code, int pos );
VOID Txt_OnChar( HWND hWnd, UINT ch, int cRepeat );
VOID Txt_OnKey( HWND hWnd, UINT vk, BOOL fDown, int cRepeat, UINT flags );
VOID Txt_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Txt_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags );
VOID Txt_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags );
VOID Txt_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Txt_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Txt_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Txt_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
LPFLOWCOLINFO WINAPI TxtGetColAtPt( LPCONTAINER lpCnt, LPPOINT lpPt, LPINT lpXstr, LPINT lpXend );

// TXTPAINT.C
VOID    WINAPI PaintTextView( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPPAINTSTRUCT lpPs );

// CNTDLG.C
BOOL    WINAPI CntStyleDlgProc( HWND, UINT, WPARAM, LPARAM );

// CNTDE16.C or CNTDE32.C
DWORD   WINAPI GetCntStyles( void );
VOID    WINAPI SetCntStyles( DWORD dwStyle );

// CNTPAINT.C
VOID GetBitmapExt( HDC hdc, HBITMAP hBitmap, LPINT xExt, LPINT yExt );
VOID DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg,
                           int xTransPxl, int yTransPxl, LPRECT lpRect );
VOID Draw3DFrame( HDC hDC, HPEN hPenHigh, HPEN hPenShadow, HPEN hPenBorder,
                  int x1, int y1, int x2, int y2, BOOL bPressed );
VOID DrawBitmap( HDC hdc, HBITMAP hBitmap, int xOrg, int yOrg, LPRECT lpRect );
BOOL WithinRect( LPRECT lpRect1, LPRECT lpRect2 );
VOID PaintInUseRect( HWND hWnd, LPCONTAINER lpCnt, HDC hDC, LPRECT lpRect );
