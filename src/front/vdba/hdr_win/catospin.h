/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : catospin.h
//
//    History:
//	10-Mar-2010 (drivi01)
//	   Clean up warnings.
//
********************************************************************/
/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CATOSPIN.H
 *
 * Public definitions for applications that use CA-Spin control:
 *
 *   -  Spin class name (used in CreateWindow call)
 *   -  Spin control styles.
 *   -  Spin notification codes.
 *   -  Spin color indices.
 *   -  Prototypes for Spin API Functions
 *
 ****************************************************************************/

#ifndef CATOSPIN_H   /* wrapper */
#define CATOSPIN_H 

/*--------------------------------------------------------------------------*
 * Class name for the spin control.
 *--------------------------------------------------------------------------*/

#ifdef _WIN32
#define CASPIN_CLASS         "CA_Spin32"
#else
#define CASPIN_CLASS         "CA_Spin"
#endif

/*--------------------------------------------------------------------------*
 * Spin control styles.
 *--------------------------------------------------------------------------*/

#define SPS_VERTICAL         0x00000001   // vertical spin button 
#define SPS_HORIZONTAL       0x00000002   // horizontal spin button
#define SPS_NOPEGNOTIFY      0x00000004   // no msgs when position is pegged
#define SPS_WRAPMINMAX       0x00000008   // spin has a wrap-around range
#define SPS_TEXTHASRANGE     0x00000010   // text contains spin range
#define SPS_INVERTCHGBTN     0x00000020   // position change buttons are inverted
#define SPS_ASYNCNOTIFY      0x00000040   // use asynchronous notification method
#define SPS_HASSTRINGS       0x00000080   // spin button has strings
#define SPS_HASSORTEDSTRS    0x00000100   // spin button has sorted strings

/*--------------------------------------------------------------------------*
 * Notification codes sent to associate window via WM_COMMAND message.
 *--------------------------------------------------------------------------*/

#define SN_ASSOCIATEGAIN     701     // wnd is now the spin associate
#define SN_ASSOCIATELOSS     702     // wnd is no longer spin associate
#define SN_RANGECHANGE       703     // the range has been changed
#define SN_POSINCREMENT      704     // the position has been incremented
#define SN_POSDECREMENT      705     // the position has been decremented
#define SN_POSWRAPTOMIN      706     // the position has wrapped to range min
#define SN_POSWRAPTOMAX      707     // the position has wrapped to range max
#define SN_POSCHANGE         708     // the position has been changed arbitrarily
#define SN_INCBUTTONUP       709     // the increment button has been released
#define SN_DECBUTTONUP       710     // the decrement button has been released

/*--------------------------------------------------------------------------*
 * Spin Color indices (passed thru SpinColorSet/Get calls).
 *--------------------------------------------------------------------------*/

#define SPC_FACE             0       // button face color
#define SPC_ARROW            1       // direction indicator
#define SPC_FRAME            2       // border around the button halves
#define SPC_HIGHLIGHT        3       // highlight color used for 3D look
#define SPC_SHADOW           4       // shadow color used for 3D look

#define SPINCOLORS           5       // total settable colors

/*--------------------------------------------------------------------------*
 * Spin Attributes (passed thru SpinAttribSet/Get/Clear calls).
 *--------------------------------------------------------------------------*/

#define SA_NOTOPARROW        0x00000001   // no arrow on the top button face
#define SA_NOBOTTOMARROW     0x00000002   // no arrow on the bottom button face
#define SA_NOLEFTARROW       0x00000004   // no arrow on the left button face
#define SA_NORIGHTARROW      0x00000008   // no arrow on the right button face



/*--------------------------------------------------------------------------*
 * Spin control API Functions
 *--------------------------------------------------------------------------*/

WORD     WINAPI SpinGetVersion( void );
HWND     WINAPI SpinAssociateSet( HWND hWnd, HWND hWndAssociate );
HWND     WINAPI SpinAssociateGet( HWND hWnd );
VOID     WINAPI SpinStyleSet( HWND hWnd, DWORD dwStyle );
DWORD    WINAPI SpinStyleGet( HWND hWnd );
VOID     WINAPI SpinStyleClear( HWND hWnd, DWORD dwStyle );
VOID     WINAPI SpinAttribSet( HWND hWnd, DWORD dwAttrib );
DWORD    WINAPI SpinAttribGet( HWND hWnd );
VOID     WINAPI SpinAttribClear( HWND hWnd, DWORD dwAttrib );
BOOL     WINAPI SpinCursorSet( HWND hSpinWnd, HCURSOR hCursor );
COLORREF WINAPI SpinColorSet( HWND hWnd, UINT iColor, COLORREF cr );
COLORREF WINAPI SpinColorGet( HWND hWnd, UINT iColor );
BOOL     WINAPI SpinRangeSet( HWND hWnd, LONG lMin, LONG lMax );
LONG     WINAPI SpinRangeMinGet( HWND hWnd );
LONG     WINAPI SpinRangeMaxGet( HWND hWnd );
LONG     WINAPI SpinPositionSet( HWND hWnd, LONG lPos );
LONG     WINAPI SpinPositionGet( HWND hWnd );
LONG     WINAPI SpinPositionInc( HWND hWnd );
LONG     WINAPI SpinPositionDec( HWND hWnd );
BOOL     WINAPI SpinTopBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp, int yOffBmp,
                               int xTransPxl, int yTransPxl, BOOL bTransparent,
                               BOOL bStretchToFit );
BOOL     WINAPI SpinBottomBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp, int yOffBmp,
                                  int xTransPxl, int yTransPxl, BOOL bTransparent,
                                  BOOL bStretchToFit );
BOOL     WINAPI SpinLeftBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp, int yOffBmp,
                                int xTransPxl, int yTransPxl, BOOL bTransparent,
                                BOOL bStretchToFit );
BOOL     WINAPI SpinRightBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp, int yOffBmp,
                                 int xTransPxl, int yTransPxl, BOOL bTransparent,
                                 BOOL bStretchToFit );
VOID     WINAPI SpinNotifyAssoc( HWND hWnd, UINT uEvent, LONG lPos );
UINT     WINAPI SpinNotifyGet( HWND hWnd, LPARAM lParam );
VOID     WINAPI SpinNotifyDone( HWND hWnd, LPARAM lParam );
LONG     WINAPI SpinSNPosGet( HWND hWnd, LPARAM lParam );
int      WINAPI SpinStrAdd( HWND hWnd, LPSTR lpszString );
int      WINAPI SpinStrDelete( HWND hWnd, int nIndex );
int      WINAPI SpinStrInsert( HWND hWnd, int nIndex, LPSTR lpszString );
int      WINAPI SpinStrFileNameAdd( HWND hWnd, UINT uAttrs, LPSTR lpszFileSpec );
int      WINAPI SpinStrPrefixFind( HWND hWnd, int nIndexStart, LPSTR lpszPrefix );
int      WINAPI SpinStrExactFind( HWND hWnd, int nIndexStart, LPSTR lpszString );
int      WINAPI SpinStrCountGet( HWND hWnd );
int      WINAPI SpinStrGet( HWND hWnd, int nIndex, LPSTR lpszString );
int      WINAPI SpinStrLenGet( HWND hWnd, int nIndex );
int      WINAPI SpinStrDataSet( HWND hWnd, int nIndex, LONG lData );
LONG     WINAPI SpinStrDataGet( HWND hWnd, int nIndex );
VOID     WINAPI SpinStrContentReset( HWND hWnd );

#endif   /* end wrapper */


