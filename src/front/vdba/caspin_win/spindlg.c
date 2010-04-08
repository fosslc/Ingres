/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * SPINDLG.C
 *
 * Routines to interface the DLL custom control to the Dialog Editor.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "spindll.h"
#include "spindlg.h"

static BOOL bDefDlgEx = FALSE;    // flag for indicating recursive dlg msg

// Local prototypes
LRESULT WINAPI SpinDefDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI StyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam );
DWORD   NEAR   FormatStyleFlags( HWND hDlg, UINT16 wWinStyle );
BOOL    NEAR   TextParmsCheck( HWND hDlg );

// Prototypes for msg cracker functions.
BOOL SpinDlg_OnInitDialog( HWND hDlg, HWND hwndFocus, LPARAM lParam );
VOID SpinDlg_OnCommand( HWND hDlg, int id, HWND hwndCtl, UINT codeNotify );

/****************************************************************************
 * SpinStyleDlgProc
 *
 * Purpose:
 *  Proc for the spin style dialog box invoked by the Dialog Editor.
 *  By utilizing the 3 macros from windowsx.h (CheckDefDlgRecursion,
 *  SetDlgMsgResult, and DefDlgProcEx ) we can make the dlg proc look like
 *  a wnd proc. This provides for more consistent looking procs and allows
 *  us to use HANDLE_MSG in the same way as wnd procs.
 *
 *  For a detailed discussion of this method see chapter 6 in the
 *  "Programming Techniques" manual from the Win32 SDK.
 *
 * Return Value:
 *  BOOL          Standard for a dialog box.
 ****************************************************************************/

BOOL WINAPI SpinStyleDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    CheckDefDlgRecursion( &bDefDlgEx );
    return SetDlgMsgResult( hDlg, iMsg, StyleDlgProc(hDlg, iMsg, wParam, lParam ));
    }

LRESULT WINAPI SpinDefDlgProc( HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    return DefDlgProcEx( hDlg, iMsg, wParam, lParam, &bDefDlgEx );
    }

/****************************************************************************
 * StyleDlgProc
 *
 * Purpose:
 *  Proc for the date style dialog which is invoked by the Dialog Editor.
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
      HANDLE_MSG( hDlg, WM_INITDIALOG, SpinDlg_OnInitDialog );
      HANDLE_MSG( hDlg, WM_COMMAND,    SpinDlg_OnCommand    );

      default:
        return SPIN_DEFDLGPROC( hDlg, iMsg, wParam, lParam );
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

BOOL SpinDlg_OnInitDialog( HWND hDlg, HWND hwndFocus, LPARAM lParam )
    {
    DWORD      dwStyle;
    UINT16     wStyle;

    dwStyle = GetSpinStyles();       // get the spin styles
    wStyle  = LOWORD(dwStyle);       // non-system styles are in loword

    if( dwStyle & WS_GROUP )
      CheckDlgButton( hDlg, IDC_CHKGROUP, 1 );

    if( dwStyle & WS_TABSTOP )
      CheckDlgButton( hDlg, IDC_CHKTABSTOP, 1 );

    if( wStyle & SPS_VERTICAL )
      CheckRadioButton( hDlg, IDC_RADIOVERTICAL,
                              IDC_RADIOHORIZONTAL, IDC_RADIOVERTICAL );

    if( wStyle & SPS_HORIZONTAL )
      CheckRadioButton( hDlg, IDC_RADIOVERTICAL,
                              IDC_RADIOHORIZONTAL, IDC_RADIOHORIZONTAL );

    if( wStyle & SPS_HASSTRINGS )
      CheckRadioButton( hDlg, IDC_RADIOHASSTRINGS,
                              IDC_RADIOHASSORTEDSTRS, IDC_RADIOHASSTRINGS );

    if( wStyle & SPS_HASSORTEDSTRS )
      CheckRadioButton( hDlg, IDC_RADIOHASSTRINGS,
                              IDC_RADIOHASSORTEDSTRS, IDC_RADIOHASSORTEDSTRS );

    CheckDlgButton( hDlg, IDC_CHKTEXTHASRANGE, 0 );
    CheckDlgButton( hDlg, IDC_CHKNOPEGNOTIFY,  0 );
    CheckDlgButton( hDlg, IDC_CHKINVERTCHGBTN, 0 );
    CheckDlgButton( hDlg, IDC_CHKWRAPMINMAX,   0 );

    if( wStyle & SPS_TEXTHASRANGE )
      CheckDlgButton( hDlg, IDC_CHKTEXTHASRANGE, 1 );

    if( wStyle & SPS_NOPEGNOTIFY )
      CheckDlgButton( hDlg, IDC_CHKNOPEGNOTIFY, 1 );

    if( wStyle & SPS_INVERTCHGBTN )
      CheckDlgButton( hDlg, IDC_CHKINVERTCHGBTN, 1 );

    if( wStyle & SPS_WRAPMINMAX )
      CheckDlgButton( hDlg, IDC_CHKWRAPMINMAX, 1 );

    return TRUE;
    }

VOID SpinDlg_OnCommand( HWND hDlg, int id, HWND hwndCtl, UINT codeNotify )
    {
    DWORD  dwStyle;
    DWORD  dwSpinStyle;
    UINT16 wWinStyle;

    switch( id )
      {
      case IDC_CHKTEXTHASRANGE:
        if( codeNotify == BN_CLICKED )
          {
          // Check for valid range text in the Text field if the
          // Text Has Range box is checked.
          if( IsDlgButtonChecked( hDlg, id ) )
            {
/*            TextParmsCheck( hDlg );*/
            CheckDlgButton( hDlg, IDC_RADIOHASSTRINGS, FALSE );
            CheckDlgButton( hDlg, IDC_RADIOHASSORTEDSTRS, FALSE );
            }
          }
        break;

      case IDC_CHKWRAPMINMAX:
        if( codeNotify == BN_CLICKED )
          {
          if( IsDlgButtonChecked( hDlg, id ) )
            CheckDlgButton( hDlg, IDC_CHKNOPEGNOTIFY, FALSE );
          }
        break;

      case IDC_CHKNOPEGNOTIFY:
        if( codeNotify == BN_CLICKED )
          {
          if( IsDlgButtonChecked(hDlg, id) )
            CheckDlgButton( hDlg, IDC_CHKWRAPMINMAX, FALSE );
          }
        break;

      case IDC_RADIOHASSTRINGS:
        if( codeNotify == BN_CLICKED )
          {
          if( IsDlgButtonChecked(hDlg, id) )
            {
            // Was checked, will now be unchecked.
            CheckDlgButton( hDlg, id, FALSE );
            }
          else
            {
            // Was unchecked, will now be checked.
            CheckDlgButton( hDlg, id, TRUE );
            CheckDlgButton( hDlg, IDC_RADIOHASSORTEDSTRS, FALSE );
            CheckDlgButton( hDlg, IDC_CHKTEXTHASRANGE, FALSE );
            }
          }
        break;

      case IDC_RADIOHASSORTEDSTRS:
        if( codeNotify == BN_CLICKED )
          {
          if( IsDlgButtonChecked(hDlg, id) )
            {
            // Was checked, will now be unchecked.
            CheckDlgButton( hDlg, id, FALSE );
            }
          else
            {
            // Was unchecked, will now be checked.
            CheckDlgButton( hDlg, id, TRUE );
            CheckDlgButton( hDlg, IDC_RADIOHASSTRINGS, FALSE );
            CheckDlgButton( hDlg, IDC_CHKTEXTHASRANGE, FALSE );
            }
          }
        break;

      case IDOK:
        // Verify the range text again.
        if( IsDlgButtonChecked( hDlg, IDC_CHKTEXTHASRANGE ) )
          {
/*          if( !TextParmsCheck( hDlg ) )*/
/*            return TRUE;*/
          }

        // Save the spin styles.
        dwSpinStyle = GetSpinStyles();
        wWinStyle   = HIWORD(dwSpinStyle);
        dwStyle     = FormatStyleFlags( hDlg, wWinStyle );
        SetSpinStyles( dwStyle );

        EndDialog( hDlg, TRUE );
        break;

      case IDCANCEL:
        EndDialog( hDlg, FALSE );
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
 *  HWND          hDlg        - handle of the style dialog
 *  UINT16        wWinStyle   - window styles
 *
 * Return Value:
 *  DWORD         Style Flags value.
 ****************************************************************************/

DWORD NEAR FormatStyleFlags( HWND hDlg, UINT16 wWinStyle )
    {
    UINT16 wStyle=0;
    DWORD  dwStyle=0L;

    dwStyle = MAKELONG(wStyle, wWinStyle);

    // the window styles use the hi-word
    dwStyle &= ~WS_GROUP;
    dwStyle &= ~WS_TABSTOP;

    if( IsDlgButtonChecked( hDlg, IDC_CHKGROUP ) )
      dwStyle |= WS_GROUP;

    if( IsDlgButtonChecked( hDlg, IDC_CHKTABSTOP ) )
      dwStyle |= WS_TABSTOP;

    if( IsDlgButtonChecked( hDlg, IDC_RADIOVERTICAL ) )
      wStyle |= SPS_VERTICAL;
    else
      wStyle |= SPS_HORIZONTAL;

    if( IsDlgButtonChecked( hDlg, IDC_RADIOHASSTRINGS ) )
      wStyle |= SPS_HASSTRINGS;
    if( IsDlgButtonChecked( hDlg, IDC_RADIOHASSORTEDSTRS ) )
      wStyle |= SPS_HASSORTEDSTRS;

    if( IsDlgButtonChecked( hDlg, IDC_CHKTEXTHASRANGE ) )
      wStyle |= SPS_TEXTHASRANGE;

    if( IsDlgButtonChecked( hDlg, IDC_CHKNOPEGNOTIFY ) )
      wStyle |= SPS_NOPEGNOTIFY;

    if( IsDlgButtonChecked( hDlg, IDC_CHKINVERTCHGBTN ) )
      wStyle |= SPS_INVERTCHGBTN;

    if( IsDlgButtonChecked( hDlg, IDC_CHKWRAPMINMAX ) )
      wStyle |= SPS_WRAPMINMAX;

    dwStyle = MAKELONG(wStyle, HIWORD(dwStyle));

    return dwStyle;
    }

/****************************************************************************
 * TextParmsCheck
 *
 * Purpose:
 *  Checks if the control text contains valid range parameters
 *  and that the initial position is within the given range.
 *
 *  NOTE: This is currently unused.
 *
 * Parameters:
 *  HWND          hDlg        - handle of the style dialog
 *
 * Return Value:
 *  BOOL          TRUE if the text is valid, FALSE otherwise.
 ****************************************************************************/

BOOL NEAR TextParmsCheck( HWND hDlg )
    {
    char  szTemp1[60];
    char  szTemp2[MAX_STRING_LEN];
    BOOL  bTextOK;
    LONG  lMin, lMax, lPos;

    // We checked the SPS_TEXTHASRANGE box, so verify that there
    // is valid text in the Text edit control.
    szTemp2[0] = 0;
    GetWindowText( GetFocus(), szTemp2, MAX_STRING_LEN );

    bTextOK = ParseSpinText( szTemp1, &lMin, &lMax, &lPos );

    if( bTextOK && (lPos < lMin || lPos > lMax) )
      {
      LoadString( hInstSpin, IDS_CLASSNAME,   szTemp1, 60 );
      LoadString( hInstSpin, IDS_RANGEPOSERR, szTemp2, 60 );
      MessageBeep(0);
      MessageBox( hDlg, szTemp2, szTemp1, MB_OK | MB_ICONEXCLAMATION );
      }

    if( !bTextOK )
      MessageBeep(0);

    return bTextOK;
    }
