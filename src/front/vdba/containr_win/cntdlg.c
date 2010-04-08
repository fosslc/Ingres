/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTDLG.C
 *
 * Routines to interface the DLL custom control to the Dialog Editor.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "cntdll.h"
#include "cntdlg.h"

static BOOL bDefDlgEx = FALSE;    // flag for indicating recursive dlg msg

// Local prototypes
LRESULT WINAPI CntDefDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI StyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam );
DWORD   NEAR   FormatStyleFlags( HWND hDlg, UINT16 wWinStyle );

// Prototypes for msg cracker functions.
BOOL CntDlg_OnInitDialog( HWND hDlg, HWND hwndFocus, LPARAM lParam );
VOID CntDlg_OnCommand( HWND hDlg, int id, HWND hwndCtl, UINT codeNotify );

/****************************************************************************
 * CntStyleDlgProc
 *
 * Purpose:
 *  Proc for the container style dialog box invoked by the Dialog Editor.
 *  By utilizing the 3 macros from windowsx.h (CheckDefDlgRecursion,
 *  SetDlgMsgResult, and DefDlgProcEx ) we can make the dlg proc look like
 *  a wnd proc. This provides for more consistent looking procs and allows
 *  us to use HANDLE_MSG in the same way as wnd procs.
 *
 *  For a detailed discussion of this method see chapter 6 in the
 *  "Programming Techniques" manual from the Win32 SDK.
 *
 * Parameters:
 *  Standard for DlgProc
 *
 * Return Value:
 *  BOOL          Standard for a dialog box.
 ****************************************************************************/

BOOL WINAPI CntStyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    CheckDefDlgRecursion( &bDefDlgEx );
    return SetDlgMsgResult( hDlg, iMsg, StyleDlgProc(hDlg, iMsg, wParam, lParam ));
    }

LRESULT WINAPI CntDefDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    return DefDlgProcEx( hDlg, iMsg, wParam, lParam, &bDefDlgEx );
    }

/****************************************************************************
 * StyleDlgProc
 *
 * Purpose:
 *  Proc for the container style dialog which is invoked by the Dialog Editor.
 *  This handles all msgs we are interested in processing for the style dlg.
 *  Any msgs not handled by us are sent to the default dialog proc.
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI StyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    switch( iMsg )
      {
      HANDLE_MSG( hDlg, WM_INITDIALOG, CntDlg_OnInitDialog );
      HANDLE_MSG( hDlg, WM_COMMAND,    CntDlg_OnCommand    );

      default:
        return CNT_DEFDLGPROC( hDlg, iMsg, wParam, lParam );
      }
    }

/****************************************************************************
 * StyleDlgProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Style DlgProc.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/

BOOL CntDlg_OnInitDialog( HWND hDlg, HWND hwndFocus, LPARAM lParam )
    {
    RECT   rect;
    DWORD  dwStyle;
    UINT16 wStyle;

    dwStyle = GetCntStyles();        // get the container styles
    wStyle  = LOWORD(dwStyle);       // non-system styles are in loword

    // Center the dialog in the screen.
    GetWindowRect( hDlg, &rect );
    OffsetRect( &rect, -rect.left, -rect.top );
    MoveWindow( hDlg, ((GetSystemMetrics(SM_CXSCREEN) - rect.right)  / 2 + 4) & ~7,
                       (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2,
                      rect.right, rect.bottom, FALSE );

    // Initialize all the checkboxes
    if( dwStyle & WS_BORDER )
      CheckDlgButton( hDlg, IDC_CHKHASBORDER, 1 );
    
    if( dwStyle & WS_DLGFRAME )
      CheckDlgButton( hDlg, IDC_CHKHASDLGFRAME, 1 );

    // caption = border | dlgframe
    if( (dwStyle & WS_BORDER) && (dwStyle & WS_DLGFRAME) )
      CheckDlgButton( hDlg, IDC_CHKHASCAPTION,  1 );

    if( dwStyle & WS_SYSMENU )
      CheckDlgButton( hDlg, IDC_CHKSYSMENU, 1 );

    if( dwStyle & WS_TABSTOP )
      CheckDlgButton( hDlg, IDC_CHKTABSTOP, 1 );

    if( dwStyle & WS_GROUP )
      CheckDlgButton( hDlg, IDC_CHKGROUP, 1 );

    if( dwStyle & WS_VISIBLE )
      CheckDlgButton( hDlg, IDC_CHKVISIBLE, 1 );

    if( wStyle & CTS_READONLY )
      CheckDlgButton( hDlg, IDC_CHKREADONLY, 1 );

    if( wStyle & CTS_SPLITBAR )
      CheckDlgButton( hDlg, IDC_CHKSPLITBAR, 1 );

    if( wStyle & CTS_VERTSCROLL )
      CheckDlgButton( hDlg, IDC_CHKVERTSCROLL, 1 );

    if( wStyle & CTS_HORZSCROLL )
      CheckDlgButton( hDlg, IDC_CHKHORZSCROLL, 1 );

    if( wStyle & CTS_INTEGRALWIDTH )
      CheckDlgButton( hDlg, IDC_CHKINTEGRALWIDTH, 1 );

    if( wStyle & CTS_INTEGRALHEIGHT )
      CheckDlgButton( hDlg, IDC_CHKINTEGRALHEIGHT, 1 );

    if( wStyle & CTS_EXPANDLASTFLD )
      CheckDlgButton( hDlg, IDC_CHKEXPANDLASTFLD, 1 );

    CheckDlgButton( hDlg, IDC_RADIOSINGLESEL,   0 );
    CheckDlgButton( hDlg, IDC_RADIOMULTIPLESEL, 0 );
    CheckDlgButton( hDlg, IDC_RADIOEXTENDEDSEL, 0 );
    CheckDlgButton( hDlg, IDC_RADIOBLOCKSEL,    0 );

    if( wStyle & CTS_SINGLESEL )
      CheckDlgButton( hDlg, IDC_RADIOSINGLESEL, 1 );
    else if( wStyle & CTS_MULTIPLESEL )
      CheckDlgButton( hDlg, IDC_RADIOMULTIPLESEL, 1 );
    else if( wStyle & CTS_EXTENDEDSEL )
      CheckDlgButton( hDlg, IDC_RADIOEXTENDEDSEL, 1 );
    else if( wStyle & CTS_BLOCKSEL )
      CheckDlgButton( hDlg, IDC_RADIOBLOCKSEL, 1 );

    return TRUE;
    }

VOID CntDlg_OnCommand( HWND hDlg, int id, HWND hwndCtl, UINT codeNotify )
    {
    DWORD  dwStyle;
    DWORD  dwCntStyle;
    UINT16 wWinStyle;

    switch( id )
      {
      case IDC_CHKHASBORDER:
        if( codeNotify == BN_CLICKED )
          {
          // if turning this off turn off caption
          if( !IsDlgButtonChecked( hDlg, id ) )
            {
            CheckDlgButton( hDlg, IDC_CHKHASCAPTION, 0 );
            }
          else
            {
            // if turning this on and dlgframe is on, turn on caption
            if( IsDlgButtonChecked( hDlg, IDC_CHKHASDLGFRAME ) )
              CheckDlgButton( hDlg, IDC_CHKHASCAPTION, 1 );
            }
          }
        break;

      case IDC_CHKHASDLGFRAME:
        if( codeNotify == BN_CLICKED )
          {
          // if turning this off turn off caption
          if( !IsDlgButtonChecked( hDlg, id ) )
            {
            CheckDlgButton( hDlg, IDC_CHKHASCAPTION, 0 );
            }
          else
            {
            // if turning this on and border is on, turn on caption
            if( IsDlgButtonChecked( hDlg, IDC_CHKHASBORDER ) )
              CheckDlgButton( hDlg, IDC_CHKHASCAPTION, 1 );
            }
          }
        break;

      case IDC_CHKHASCAPTION:
        if( codeNotify == BN_CLICKED )
          {
          // if turning this on turn on border and dlgframe
          if( IsDlgButtonChecked( hDlg, id ) )
            {
            CheckDlgButton( hDlg, IDC_CHKHASBORDER,   1 );
            CheckDlgButton( hDlg, IDC_CHKHASDLGFRAME, 1 );
            }
          else
            {
            // if turning this off turn off border and dlgframe
            CheckDlgButton( hDlg, IDC_CHKHASBORDER,   0 );
            CheckDlgButton( hDlg, IDC_CHKHASDLGFRAME, 0 );
            }
          }
        break;

      case IDC_RADIOSINGLESEL:
        if( codeNotify == BN_CLICKED )
          {
          // if already on, turn it off; else set it
          if( IsDlgButtonChecked( hDlg, id ) )
            CheckDlgButton( hDlg, IDC_RADIOSINGLESEL, 0 );
          else
            CheckRadioButton( hDlg, IDC_RADIOSINGLESEL, 
                              IDC_RADIOBLOCKSEL, IDC_RADIOSINGLESEL );
          }
        break;

      case IDC_RADIOMULTIPLESEL:
        if( codeNotify == BN_CLICKED )
          {
          // if already on, turn it off; else set it
          if( IsDlgButtonChecked( hDlg, id ) )
            CheckDlgButton( hDlg, IDC_RADIOMULTIPLESEL, 0 );
          else
            CheckRadioButton( hDlg, IDC_RADIOSINGLESEL, 
                              IDC_RADIOBLOCKSEL, IDC_RADIOMULTIPLESEL );
          }
        break;

      case IDC_RADIOEXTENDEDSEL:
        if( codeNotify == BN_CLICKED )
          {
          // if already on, turn it off; else set it
          if( IsDlgButtonChecked( hDlg, id ) )
            CheckDlgButton( hDlg, IDC_RADIOEXTENDEDSEL, 0 );
          else
            CheckRadioButton( hDlg, IDC_RADIOSINGLESEL, 
                              IDC_RADIOBLOCKSEL, IDC_RADIOEXTENDEDSEL );
          }
        break;

      case IDC_RADIOBLOCKSEL:
        if( codeNotify == BN_CLICKED )
          {
          // if already on, turn it off; else set it
          if( IsDlgButtonChecked( hDlg, id ) )
            CheckDlgButton( hDlg, IDC_RADIOBLOCKSEL, 0 );
          else
            CheckRadioButton( hDlg, IDC_RADIOSINGLESEL, 
                              IDC_RADIOBLOCKSEL, IDC_RADIOBLOCKSEL );
          }
        break;

      case IDOK:
        // Save the container styles.
        dwCntStyle = GetCntStyles();
        wWinStyle  = HIWORD(dwCntStyle);
        dwStyle   = FormatStyleFlags( hDlg, wWinStyle );
        SetCntStyles( dwStyle );

        EndDialog( hDlg, TRUE );
        break;

      case IDCANCEL:
        EndDialog( hDlg, FALSE );
        break;

      default:
        break;
      }

    return;
    }

/****************************************************************************
 * FormatStyleFlags
 *
 * Purpose:
 *  Returns a DWORD with flags set to whatever options are checked in
 *  the dialog box.  It is up to the implementer of the dialog box to
 *  code the exact bit fields in this function.
 *
 * Parameters:
 *  HWND          hDlg        - wnd handle of style dialog
 *  UINT16        wWinStyle   - window styles
 *
 * Return Value:
 *  DWORD         Style Flags value.
 ****************************************************************************/

DWORD NEAR FormatStyleFlags( HWND hDlg, UINT16 wWinStyle )
    {
    UINT16  wStyle=0;
    DWORD   dwStyle=0L;

    dwStyle = MAKELONG(wStyle, wWinStyle);

    // the window styles use the hi-word
    dwStyle &= ~WS_SYSMENU;

    // caption = border | dlgframe
    if( IsDlgButtonChecked( hDlg, IDC_CHKHASCAPTION ) )
      {
      dwStyle |= WS_CAPTION;
      }
    else    
      {
      dwStyle &= ~WS_CAPTION;

      if( IsDlgButtonChecked( hDlg, IDC_CHKHASBORDER ) )
        dwStyle |= WS_BORDER;
      else    
        dwStyle &= ~WS_BORDER;

      if( IsDlgButtonChecked( hDlg, IDC_CHKHASDLGFRAME ) )
        dwStyle |= WS_DLGFRAME;
      else    
        dwStyle &= ~WS_DLGFRAME;
      }

    if( IsDlgButtonChecked( hDlg, IDC_CHKSYSMENU ) )
      dwStyle |= WS_SYSMENU;
    else
      dwStyle &= ~WS_SYSMENU;

    if( IsDlgButtonChecked( hDlg, IDC_CHKTABSTOP ) )
      dwStyle |= WS_TABSTOP;
    else
      dwStyle &= ~WS_TABSTOP;

    if( IsDlgButtonChecked( hDlg, IDC_CHKGROUP ) )
      dwStyle |= WS_GROUP;
    else
      dwStyle &= ~WS_GROUP;

//    if( IsDlgButtonChecked( hDlg, IDC_CHKVISIBLE ) )
//      dwStyle |= WS_VISIBLE;
//    else
//      dwStyle &= ~WS_VISIBLE;

    // the control styles use the lo-word
    if( IsDlgButtonChecked( hDlg, IDC_CHKREADONLY ) )
      wStyle |= CTS_READONLY;

    if( IsDlgButtonChecked( hDlg, IDC_CHKSPLITBAR ) )
      wStyle |= CTS_SPLITBAR;

    if( IsDlgButtonChecked( hDlg, IDC_RADIOSINGLESEL ) )
      wStyle |= CTS_SINGLESEL;
    else if( IsDlgButtonChecked( hDlg, IDC_RADIOMULTIPLESEL ) )
      wStyle |= CTS_MULTIPLESEL;
    else if( IsDlgButtonChecked( hDlg, IDC_RADIOEXTENDEDSEL ) )
      wStyle |= CTS_EXTENDEDSEL;
    else if( IsDlgButtonChecked( hDlg, IDC_RADIOBLOCKSEL ) )
      wStyle |= CTS_BLOCKSEL;

    if( IsDlgButtonChecked( hDlg, IDC_CHKVERTSCROLL ) )
      wStyle |= CTS_VERTSCROLL;

    if( IsDlgButtonChecked( hDlg, IDC_CHKHORZSCROLL ) )
      wStyle |= CTS_HORZSCROLL;

    if( IsDlgButtonChecked( hDlg, IDC_CHKINTEGRALWIDTH ) )
      wStyle |= CTS_INTEGRALWIDTH;

    if( IsDlgButtonChecked( hDlg, IDC_CHKINTEGRALHEIGHT ) )
      wStyle |= CTS_INTEGRALHEIGHT;

    if( IsDlgButtonChecked( hDlg, IDC_CHKEXPANDLASTFLD ) )
      wStyle |= CTS_EXPANDLASTFLD;

    dwStyle = MAKELONG(wStyle, HIWORD(dwStyle));

    return dwStyle;
    }
