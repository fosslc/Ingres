/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * SPINDLL.H
 *
 * DLL Specific include file.
 *
 * Contains all definitions and and prototypes pertinent ONLY
 * to the DLL.  API related information is contained in CATOSPIN.H.
 ****************************************************************************/

#ifndef CA_Spin_H
#define CA_Spin_H

#include <custcntl.h>   // Standard Windows header file for custom controls.
#include "catospin.h"   // Get interface stuff for the control.

HINSTANCE hInstSpin;    // Spin instance handle.

#define IDC_LISTBOX     0xFF12  // ID of internal listbox

// This macro extracts the Spin struct pointer from the spin wnd handle.
#define GETSPIN(hw)       ((LPSPIN) GetWindowLong((hw), GWL_SPINHMEM ))

#define SPIN_DEFWNDPROC   DefWindowProc    // default proc for main spin wndproc
#define SPIN_DEFDLGPROC   SpinDefDlgProc   // default proc for spin dlgproc

/*--------------------------------------------------------------------------*
 * Spin version number
 *--------------------------------------------------------------------------*/

#define SPIN_VERSION      0x0100

/*--------------------------------------------------------------------------*
 * Spin Notification Event structure (used with SPS_ASYNCNOTIFY style)
 *--------------------------------------------------------------------------*/

typedef struct tagSNEVENT
    {
    UINT     uCurNotify;       // current notification value
    LONG     lCurPos;          // current position
    } SNEVENT;

typedef SNEVENT      *PSNEVENT;
typedef SNEVENT FAR *LPSNEVENT;

#define LEN_SNEVENT sizeof(SNEVENT)

/*--------------------------------------------------------------------------*
 * SPIN struct - global handle to this is stored the window extra bytes.
 *--------------------------------------------------------------------------*/

typedef unsigned short UINT16;   // type for 16 bit unsigned quantities

typedef struct tagSPIN
    {
    HWND        hWndAssoc;        // Associate window handle
    HWND        hWndLB;           // internal listbox window handle
    DWORD       flSpinAttr;       // spin button attributes
    HCURSOR     hCursor;          // handle to spin cursor
    DWORD       dwStyle;          // Copy of GetWindowLong(hWnd, GWL_STYLE)
    LONG        lMin;             // Minimum position
    LONG        lMax;             // Maximum position
    LONG        lPos;             // Current position

    UINT        uCurNotify;       // current notification value
    LONG        lCurPos;          // current position for current notification

    HBITMAP     hBmpInc;          // user bitmap for drawing increment arrow
    int         xOffBmpInc;       // horz offset for increment bitmap
    int         yOffBmpInc;       // vert offset for increment bitmap
    int         xTransPxlInc;     // xpos of transparent bkgrnd pixel for increment bitmap 
    int         yTransPxlInc;     // ypos of transparent bkgrnd pixel for increment bitmap 
    BOOL        bIncBmpTrans;     // user increment bmp has transparent bkgrnd

    HBITMAP     hBmpDec;          // user bitmap for drawing decrement arrow
    int         xOffBmpDec;       // horz offset for decrement bitmap
    int         yOffBmpDec;       // vert offset for decrement bitmap
    int         xTransPxlDec;     // xpos of transparent bkgrnd pixel for decrement bitmap 
    int         yTransPxlDec;     // ypos of transparent bkgrnd pixel for decrement bitmap 
    BOOL        bDecBmpTrans;     // user decrement bmp has transparent bkgrnd

    UINT        uState;           // State flags
    COLORREF    crSpin[SPINCOLORS]; // configurable spin colors
    HPEN        hPen[SPINCOLORS];   // spin colors for line drawing
    } SPIN;

typedef SPIN     *P_SPIN;
typedef SPIN FAR *LPSPIN;

#define LEN_SPIN sizeof(SPIN)

/*--------------------------------------------------------------------------*
 * Spin class/window extra bytes.
 *--------------------------------------------------------------------------*/

// Number of extra bytes for the spin class and window.
#define LEN_CLASSEXTRA           0
#define LEN_WINDOWEXTRA         16
   
// Offsets to use with GetWindowWord
#define GWW_SPINHMEM             0
#define GWL_SPINHMEM             0

/*--------------------------------------------------------------------------*
 * Spin state flags.
 *--------------------------------------------------------------------------*/

#define SPSTATE_HIDDEN          0x0001
#define SPSTATE_MOUSEOUT        0x0002
#define SPSTATE_UPCLICK         0x0004
#define SPSTATE_DOWNCLICK       0x0008
#define SPSTATE_LEFTCLICK       0x0004  // Repeated since SPS_VERTICAL and
#define SPSTATE_RIGHTCLICK      0x0008  // SPS_HORIZONTAL are exclusive.
#define SPSTATE_UPGRAYED        0x0010
#define SPSTATE_DOWNGRAYED      0x0020
#define SPSTATE_LEFTGRAYED      0x0010  // Repeated since SPS_VERTICAL and
#define SPSTATE_RIGHTGRAYED     0x0020  // SPS_HORIZONTAL are exclusive.
#define SPSTATE_SENTNOTIFY      0x0040

#define SPSTATE_ALL             0x007F  // Combination of state flags.

// Combination of grayed states.
#define SPSTATE_GRAYED          (SPSTATE_LEFTGRAYED | SPSTATE_RIGHTGRAYED)

// Combination of click states.
#define SPSTATE_CLICKED         (SPSTATE_LEFTCLICK | SPSTATE_RIGHTCLICK)

/*--------------------------------------------------------------------------*
 * Macros to change the control state given an LPSPIN and state flags.
 *--------------------------------------------------------------------------*/

#define StateSet(lp, uFlags)    (lp->uState |=  (uFlags))
#define StateClear(lp, uFlags)  (lp->uState &= ~(uFlags))
#define StateTest(lp, uFlags)   (lp->uState &   (uFlags))

/*--------------------------------------------------------------------------*
 * Default range and position constants.
 *--------------------------------------------------------------------------*/

#define IDEFAULTMIN              1
#define IDEFAULTMAX              9
#define IDEFAULTPOS              5

/*--------------------------------------------------------------------------*
 * Timer identifiers
 *--------------------------------------------------------------------------*/

#define IDT_FIRSTCLICK         500
#define IDT_HOLDCLICK          501
#define CTICKS_FIRSTCLICK      400
#define CTICKS_HOLDCLICK        50

/*--------------------------------------------------------------------------*
 * Misc constants
 *--------------------------------------------------------------------------*/

#define MAX_STRING_LEN         256    // max len of string data per tab
#define CLR_INIT                 0    // flag for initializing colors
#define CLR_UPDATE               1    // flag for updating colors
#define CLR_DELETE               2    // flag for deleting color objects

/*--------------------------------------------------------------------------*
 * Stringtable identifiers
 *--------------------------------------------------------------------------*/

#define IDS_CLASSNAME            0
#define IDS_FULLNAME             1
#define IDS_SHORTNAME            2
#define IDS_VERTICAL             3
#define IDS_HORIZONTAL           4
#define IDS_SPS_TABSTOP          5 
#define IDS_SPS_GROUP            6 
#define IDS_SPS_VERTICAL         7
#define IDS_SPS_HORIZONTAL       8
#define IDS_SPS_TEXTHASRANGE     9
#define IDS_SPS_NOPEGNOTIFY     10 
#define IDS_SPS_INVERTCHGBTN    11
#define IDS_SPS_WRAPMINMAX      12
#define IDS_SPS_HASSTRINGS      13
#define IDS_SPS_HASSORTEDSTRS   14
#define IDS_RANGEMAXERR         15
#define IDS_RANGEPOSERR         16
#define IDS_INVALIDPARMS        17

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

// SPINWEP.C
int     WINAPI WEP( int );

// SPININIT.C
int     WINAPI LibMain( HINSTANCE hInstance, UINT wDataSeg, UINT cbHeapSize, LPSTR lpCmdLine );

// CATOSPIN.C
LRESULT WINAPI SpinWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam );
VOID    WINAPI NotifyAssocWnd( HWND hWnd, LPSPIN lpSpin, UINT uEvent, LONG lPos );
BOOL    WINAPI ParseSpinText( LPSTR lpsz, LPLONG lplMin, LPLONG lplMax, LPLONG lplPos );
VOID    WINAPI PositionChange( HWND hWnd, LPSPIN lpSpin );
VOID    WINAPI UpdateSpin( HWND hWnd, LPRECT lpRect, BOOL bEraseBkGrnd );

// SPINPNT.C
VOID    WINAPI PaintSpinButton( HWND hWnd, LPSPIN lpSpin, HDC hDC, LPPAINTSTRUCT lpPs );
VOID    WINAPI SpinColorManager( HWND hWnd, LPSPIN lpSpin, UINT uAction );

// SPINDLG.C
BOOL    WINAPI SpinStyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam );

// SPINDE16.C or SPINDE32.C
DWORD   WINAPI GetSpinStyles( void );
VOID    WINAPI SetSpinStyles( DWORD dwStyle );

#endif  /* CA_Spin_H */
