/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CATOSPIN.C
 *
 * Contains the main window procedure for the Spin control.
 *
 * History:
 *	07-May-2009 (drivi01)
 *	     Redefine CCSTYLE to be C_CCSTYLE b/c of conflict
 *	     during Visual Studio 2008 port.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "spindll.h"
#ifdef REALIZER
#include "custctrl.h"                // Realizer messaging stuff
#endif

VOID NEAR ClickedRectCalc( HWND hWnd, LPSPIN lpSpin, LPRECT lpRect );
VOID WINAPI PositionChange( HWND hWnd, LPSPIN lpSpin );
BOOL NEAR WTranslateUpToChar( LPSTR FAR *lplpsz, LPLONG lpnVal );

// Prototypes for SpinWndProc msg cracker functions.
BOOL Spin_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Spin_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
VOID Spin_OnSize( HWND hWnd, UINT state, int cx, int cy );
VOID Spin_OnNCDestroy( HWND hWnd );
BOOL Spin_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Spin_OnPaint( HWND hWnd );
VOID Spin_OnEnable( HWND hWnd, BOOL fEnable );
VOID Spin_OnShowWindow( HWND hWnd, BOOL fShow, UINT status );
VOID Spin_OnCancelMode( HWND hWnd );
VOID Spin_OnTimer( HWND hWnd, UINT id );
VOID Spin_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Spin_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags );
VOID Spin_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags );

/****************************************************************************
 * SpinWndProc
 *
 * Purpose:
 *  Main Window Procedure for the Spin custom control.
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI SpinWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    switch( iMsg )
      {
      HANDLE_MSG( hWnd, WM_NCCREATE,       Spin_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Spin_OnCreate        );
      HANDLE_MSG( hWnd, WM_SIZE,           Spin_OnSize          );
      HANDLE_MSG( hWnd, WM_NCDESTROY,      Spin_OnNCDestroy     );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Spin_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_PAINT,          Spin_OnPaint         );
      HANDLE_MSG( hWnd, WM_ENABLE,         Spin_OnEnable        );
      HANDLE_MSG( hWnd, WM_SHOWWINDOW,     Spin_OnShowWindow    );
      HANDLE_MSG( hWnd, WM_CANCELMODE,     Spin_OnCancelMode    );
      HANDLE_MSG( hWnd, WM_TIMER,          Spin_OnTimer         );
      HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK,  Spin_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Spin_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_MOUSEMOVE,      Spin_OnMouseMove     );
      HANDLE_MSG( hWnd, WM_LBUTTONUP,      Spin_OnLButtonUp     );

#ifdef REALIZER
     /*---------------------------------------------------------------------*
      * REALIZER Message processing
      *---------------------------------------------------------------------*/

      // This is a special message sent only from Realizer. Its purpose
      // is to allow us to 'steal' certain keystrokes that normally the
      // spin (or any other control) would not receive.
#ifdef ZOT
      case WM_KEYDOWN + 0x1000:
        switch( wParam )
          {
          case VK_TAB:
          case VK_ESCAPE:
          case VK_RETURN:
          case VK_INSERT:
            {
            // Get a pointer to the SPIN structure for this control.
            LPSPIN lpSpin = GETSPIN( hWnd );

            // Tell Realizer that we are going to process this keystroke.
            // Realizer should have registered as the AUX window.
            if( lpSpin->hAuxWnd )
              {
              SendMessage( lpSpin->hAuxWnd, WM_USER+2, 0, 0L );

              // Now send the keystroke to the spin window.
              FORWARD_WM_KEYDOWN( GetFocus(), wParam, 0, 0, SendMessage );
              }
            }
            break;
          }
        break;
#endif /* ZOT */

      case CCM_SETNUM:
        {
        LPLONG lplStyles=(LPLONG)lParam;
        LONG   lStyle=0L;
        int    i;

        // For SETNUM with the spin we only care about the styles.
        if( *lplStyles == C_CCSTYLE )   
          {
          // For Realizer style types, the first wParam is a count of 
          // style modifiers, and lParam points to an array of longs 
          // containing the individual style flags.

          // Eat the CCSTYLE type and get to the real styles.
          lplStyles++;

          // Collect all the styles being passed in.
          for( i = 1; i < (int) wParam; i++, lplStyles++ )
            lStyle |= *lplStyles;

          // Apply these styles to the spin.
          SpinStyleSet( hWnd, (DWORD)lStyle );
          return 1L;
          }
        }
        break;

      case CCM_QDEFAULTSIZE:
      case CCM_GETNUM:
      case CCM_GETSTRCOUNT:
      case CCM_GETSTRLEN:
      case CCM_GETSTR:
      case CCM_FORMSIZED:
      case CCM_SETSTR:
      case CCM_RESETCONTENT:
        break;
      case CCM_GETFLAGS:
        return CCF_FOCUS; 

#endif  /* REALIZER */

      default:
        return SPIN_DEFWNDPROC( hWnd, iMsg, wParam, lParam );
      }

    return 0L;
    }

/****************************************************************************
 * SpinWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the SpinWndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Values:
 *  Varies according to the message being processed.
 ****************************************************************************/

BOOL Spin_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPSPIN lpNewSpin;

    // Alloc memory for the control's SPIN struct
    // and store the handle in the window's extra bytes.
    lpNewSpin = (LPSPIN) GlobalAllocPtr( GHND, LEN_SPIN );
    if( lpNewSpin == NULL )
      return 0L;

    // Save the spin pointer in the window extra bytes.
    SetWindowLong( hWnd, GWL_SPINHMEM, (LONG)lpNewSpin );

    return FORWARD_WM_NCCREATE( hWnd, lpCreateStruct, SPIN_DEFWNDPROC );
    }

BOOL Spin_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );
    DWORD  dwLBstyle;

    // Our associate is the parent by default.
    lpSpin->hWndAssoc = GetParent( hWnd );

    // Store styles in the spin struct.
    lpSpin->dwStyle = lpCreateStruct->style;

    // If neither or both styles are set use vertical.
    if( !(SPS_VERTICAL & lpSpin->dwStyle) && !(SPS_HORIZONTAL & lpSpin->dwStyle) )
      lpSpin->dwStyle |= SPS_VERTICAL;
    else if( (SPS_VERTICAL & lpSpin->dwStyle) && (SPS_HORIZONTAL & lpSpin->dwStyle) )
      lpSpin->dwStyle &= ~SPS_HORIZONTAL;

    // Check for spin string styles.
    if( (lpSpin->dwStyle & SPS_HASSTRINGS) || (lpSpin->dwStyle & SPS_HASSORTEDSTRS) )
      {
      // If trying to set both string styles, use sorted strings.
      if( (lpSpin->dwStyle & SPS_HASSTRINGS) && (lpSpin->dwStyle & SPS_HASSORTEDSTRS) )
        lpSpin->dwStyle &= ~SPS_HASSTRINGS;

      // Remove text-has-range style if using strings.
      lpSpin->dwStyle &= ~SPS_TEXTHASRANGE;

      // Turn on the listbox sort style if user wants it.
      dwLBstyle = WS_CHILD | LBS_HASSTRINGS;
      if( lpSpin->dwStyle & SPS_HASSORTEDSTRS )
        dwLBstyle |= LBS_SORT;

      // Create the invisible listbox to manage the spin button strings.
      lpSpin->hWndLB = CreateWindow( "listbox", NULL, dwLBstyle, 0, 0, 20, 20,
                                      hWnd, (HMENU)IDC_LISTBOX, hInstSpin, 0L );

      // If the create failed don't apply the string style.
      if( !lpSpin->hWndLB )
        lpSpin->dwStyle &= ~(SPS_HASSTRINGS | SPS_HASSORTEDSTRS);
      }

    if( lpSpin->hWndLB )
      {
      // When strings are enabled the range is handled internally.
      lpSpin->lMin = 0;
      lpSpin->lMax = 0;
      lpSpin->lPos = 0;
      }
    else
      {
      // Use defaults for spin range when not using strings.
      lpSpin->lMin = IDEFAULTMIN;
      lpSpin->lMax = IDEFAULTMAX;
      lpSpin->lPos = IDEFAULTPOS;
      }

    // Set the arrow as the default cursor.
    lpSpin->hCursor = LoadCursor( NULL, IDC_ARROW );

    // Clear out all initial states.
    StateClear(lpSpin, SPSTATE_ALL);

    // If setting no-peg see if we have to gray either button half.
    if( SPS_NOPEGNOTIFY & lpSpin->dwStyle )
      {
      if( lpSpin->lPos == lpSpin->lMax )
        {
        if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        else
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        }

      if( lpSpin->lPos == lpSpin->lMin )
        {
        if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        else
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        }
      }

    // Initialize the spin colors.
    SpinColorManager( hWnd, lpSpin, CLR_INIT );

    // NOTE: When using a msg cracker for WM_CREATE we must
    //       return TRUE if all is well instead of 0.
    return TRUE;
    }

VOID Spin_OnSize( HWND hWnd, UINT state, int cx, int cy )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );
    char szString[MAX_STRING_LEN];
    LONG lMin, lMax, lPos;
    BOOL bTextParms=FALSE;
    BOOL bRangeOK=TRUE;
    BOOL bPosOK=TRUE;

    if( lpSpin->dwStyle & SPS_TEXTHASRANGE )
      {
      // Parse the control text for tab sizes, statuslines, and backpages.
      szString[0] = 0;
      GetWindowText( hWnd, szString, MAX_STRING_LEN );

      bTextParms = ParseSpinText( szString, &lMin, &lMax, &lPos);

      if( bTextParms )
        {
        // Verify that the position is in the given range.
        if( lMin > lMax )
          {
          bRangeOK = FALSE;
          LoadString( hInstSpin, IDS_RANGEMAXERR, szString, MAX_STRING_LEN );
          MessageBeep(0);
          MessageBox( hWnd, szString, "CA-SPIN", MB_OK | MB_ICONEXCLAMATION );
          }

        // Verify that the position is in the given range.
        if( bRangeOK )
          {
          if( lPos < lMin || lPos > lMax )
            {
            bPosOK = FALSE;
            LoadString( hInstSpin, IDS_RANGEPOSERR, szString, MAX_STRING_LEN );
            MessageBeep(0);
            MessageBox( hWnd, szString, "CA-SPIN", MB_OK | MB_ICONEXCLAMATION );
            }
          }

        // Set the values into the struct.
        if( bRangeOK && bPosOK )
          {
          lpSpin->lMin = lMin;
          lpSpin->lMax = lMax;
          lpSpin->lPos = lPos;
          }
        }
      else
        {
        LoadString( hInstSpin, IDS_INVALIDPARMS, szString, MAX_STRING_LEN );
        MessageBeep(0);
        MessageBox( hWnd, szString, "CA-SPIN", MB_OK | MB_ICONEXCLAMATION );
        }
      }

    return;
    }

VOID Spin_OnNCDestroy( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );

    // Delete the spin button color objects.
    SpinColorManager( hWnd, lpSpin, CLR_DELETE );

    // Free all the memory for this spin button.
    GlobalFreePtr( lpSpin );
    return;
    }

BOOL Spin_OnEraseBkgnd( HWND hWnd, HDC hDC )
    {
    // Eat this message to avoid erasing portions that
    // we are going to repaint in WM_PAINT.
    return TRUE;
    }

VOID Spin_OnPaint( HWND hWnd )
    {
    LPSPIN      lpSpin = GETSPIN( hWnd );
    PAINTSTRUCT ps;
    HDC         hDC;

    hDC = BeginPaint( hWnd, &ps );
   
    PaintSpinButton( hWnd, lpSpin, hDC, &ps );

    EndPaint( hWnd, &ps );

    return;
    }

VOID Spin_OnEnable( HWND hWnd, BOOL fEnable )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );

    if( fEnable )
      {
      StateClear(lpSpin, SPSTATE_GRAYED);

      if( SPS_NOPEGNOTIFY & lpSpin->dwStyle )
        {
        if( lpSpin->lPos == lpSpin->lMax )
          {
          if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
            StateSet(lpSpin, SPSTATE_UPGRAYED);
          else
            StateSet(lpSpin, SPSTATE_DOWNGRAYED);
          }

        if( lpSpin->lPos == lpSpin->lMin )
          {
          if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
            StateSet(lpSpin, SPSTATE_DOWNGRAYED);
          else
            StateSet(lpSpin, SPSTATE_UPGRAYED);
          }
        }
      }
    else
      {
      StateSet(lpSpin, SPSTATE_GRAYED);
      }
 
    // Force a repaint since the control will look different.
    UpdateSpin( hWnd, NULL, TRUE );
    }

VOID Spin_OnShowWindow( HWND hWnd, BOOL fShow, UINT status )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );

    // Set or clear the hidden flag. Windows will automatically
    // force a repaint if we become visible.
    if( fShow )
      StateClear(lpSpin, SPSTATE_HIDDEN);
    else
      StateSet(lpSpin, SPSTATE_HIDDEN);
    }

VOID Spin_OnCancelMode( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );

   /*-------------------------------------------------------------------*
    * IMPORTANT MESSAGE!  WM_CANCELMODE means that a dialog or some
    * other modal process has started. We must make sure that we cancel
    * any clicked state we are in and release the capture.
    *-------------------------------------------------------------------*/

    StateClear( lpSpin, SPSTATE_DOWNCLICK | SPSTATE_UPCLICK );
    KillTimer( hWnd, IDT_FIRSTCLICK );
    KillTimer( hWnd, IDT_HOLDCLICK );
    ReleaseCapture();
    return;
    }

VOID Spin_OnTimer( HWND hWnd, UINT id )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );

    // We run two timers:  
    //   - the 1st is the initial delay after the
    //     first click before we begin repeating
    //   - the 2nd is the repeat rate.
    if( id == IDT_FIRSTCLICK )
      {
      KillTimer( hWnd, id );
      SetTimer( hWnd, IDT_HOLDCLICK, CTICKS_HOLDCLICK, NULL );
      }

    // Send a new scroll message if the mouse is still in the
    // originally clicked area.
    if( !StateTest(lpSpin, SPSTATE_MOUSEOUT) )
      PositionChange( hWnd, lpSpin );
    }

VOID Spin_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );
    RECT   rect;
    UINT   uState;
    int    cx, cy;

   /*-----------------------------------------------------------------------*
    * When we get a mouse down message, we know that the mouse is over the
    * control.  We then do the following steps to set up the new state:
    *   1 - Do a hit-test to determine which button half was clicked.
    *   2 - Set appropriate SPSTATE_*CLICK state and repaint the clicked half.
    *   3 - Send an initial scroll message.
    *   4 - Set the mouse capture.
    *   5 - Set the initial delay timer before repeating the scroll message.
    *   
    * A WM_LBUTTONDBLCLK message means that the user clicked the control
    * twice in succession which we want to treat like WM_LBUTTONDOWN. This is
    * safe since we will receive a WM_LBUTTONUP before the WM_LBUTTONDBLCLK.
    *-----------------------------------------------------------------------*/

    // Only need to hit-test the upper half for a vertical
    // control or the left half for a horizontal control.
    GetClientRect( hWnd, &rect );
    cx = rect.right  >> 1;
    cy = rect.bottom >> 1;

    if( SPS_VERTICAL & lpSpin->dwStyle )
      uState = ( y > cy ) ? SPSTATE_DOWNCLICK  : SPSTATE_UPCLICK;
    else
      uState = ( x > cx ) ? SPSTATE_RIGHTCLICK : SPSTATE_LEFTCLICK;

    // If the clicked-on half is grayed throw away the buttondown.
    if( StateTest(lpSpin, SPSTATE_GRAYED) )
      {
      if( StateTest(lpSpin, SPSTATE_UPGRAYED) && uState == SPSTATE_UPCLICK )
        return;
      if( StateTest(lpSpin, SPSTATE_DOWNGRAYED) && uState == SPSTATE_DOWNCLICK )
        return;
      }

    // Change-state-and-repaint
    StateClear( lpSpin, SPSTATE_SENTNOTIFY );
    StateSet( lpSpin, uState );
    ClickedRectCalc( hWnd, lpSpin, &rect );
    InvalidateRect( hWnd, &rect, TRUE );
    UpdateWindow( hWnd );

    PositionChange( hWnd, lpSpin );

    SetCapture( hWnd );
    SetTimer( hWnd, IDT_FIRSTCLICK, CTICKS_FIRSTCLICK, NULL );
    }

VOID Spin_OnMouseMove( HWND hWnd, int x, int y, UINT keyFlags )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );
    POINT  pt;
    RECT   rect;
    UINT   uState;

   /*-----------------------------------------------------------------------*
    * On WM_MOUSEMOVE messages we want to know if the mouse has moved out of
    * the control when the control is in a clicked state. If the control has
    * not been clicked, then we have nothing to do. Otherwise we want to set
    * the SPSTATE_MOUSEOUT flag and repaint so the button visually comes up.
    *-----------------------------------------------------------------------*/

    // Set the spin button cursor.
    SetCursor( lpSpin->hCursor );

    if( !StateTest(lpSpin, SPSTATE_CLICKED) )
      return;

    // Set the mouse coord into a POINT.
    pt.x = x;
    pt.y = y;

    // Get the area we originally clicked and the new POINT.
    ClickedRectCalc( hWnd, lpSpin, &rect );

    uState = lpSpin->uState;

    // Hit-Test the rectange and change the state if necessary.
    if( PtInRect( &rect, pt ) )
      StateClear(lpSpin, SPSTATE_MOUSEOUT);
    else
      StateSet(lpSpin, SPSTATE_MOUSEOUT);

    // If the state changed, repaint the appropriate part of the control.
    if( uState != lpSpin->uState )
      {
      InvalidateRect( hWnd, &rect, TRUE );
      UpdateWindow( hWnd );
      }
    }

VOID Spin_OnLButtonUp( HWND hWnd, int x, int y, UINT keyFlags )
    {
    LPSPIN lpSpin = GETSPIN( hWnd );
    RECT   rect;
    BOOL   bSentDec=FALSE;
    BOOL   bGrayChg=FALSE;

   /*-----------------------------------------------------------------------*
    * A mouse button up event is much like WM_CANCELMODE since we have to
    * clean out whatever state the control is in:
    *  1 - Kill any repeat timers we might have created.
    *  2 - Release the mouse capture.
    *  3 - Clear the clicked states and repaint.
    *-----------------------------------------------------------------------*/

    KillTimer( hWnd, IDT_FIRSTCLICK );
    KillTimer( hWnd, IDT_HOLDCLICK );
    ReleaseCapture();

    // Adjust the states if we are clicked even if the mouse is outside the
    // boundaries of the control. 
/*    if( StateTest(lpSpin, SPSTATE_CLICKED) && !StateTest(lpSpin, SPSTATE_MOUSEOUT) )*/
    if( StateTest(lpSpin, SPSTATE_CLICKED) )
      {
      // Calculate the rectangle before clearing states.
      ClickedRectCalc( hWnd, lpSpin, &rect );

      // Remember if we sent an increment or decrement notification.
      if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
        {
        if( StateTest(lpSpin, SPSTATE_DOWNCLICK | SPSTATE_RIGHTCLICK) )
          bSentDec = TRUE;
        }
      else
        {
        if( StateTest(lpSpin, SPSTATE_UPCLICK | SPSTATE_LEFTCLICK) )
          bSentDec = TRUE;
        }

      // Clear the states so we repaint properly.
      StateClear(lpSpin, SPSTATE_MOUSEOUT);
      StateClear(lpSpin, SPSTATE_CLICKED);

      // See if we have to gray or ungray either button half.
      if( SPS_NOPEGNOTIFY & lpSpin->dwStyle )
        {
        // If one half was grayed it will no longer be grayed.
        if( StateTest(lpSpin, SPSTATE_GRAYED) )
          {
          StateClear(lpSpin, SPSTATE_GRAYED);
          bGrayChg = TRUE;
          }
        
        if( lpSpin->lPos == lpSpin->lMax )
          {
          if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
            StateSet(lpSpin, SPSTATE_UPGRAYED);
          else
            StateSet(lpSpin, SPSTATE_DOWNGRAYED);
          bGrayChg = TRUE;
          }

        if( lpSpin->lPos == lpSpin->lMin )
          {
          if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
            StateSet(lpSpin, SPSTATE_DOWNGRAYED);
          else
            StateSet(lpSpin, SPSTATE_UPGRAYED);
          bGrayChg = TRUE;
          }
        }

      if( bGrayChg )
        InvalidateRect( hWnd, NULL, TRUE );
      else
        InvalidateRect( hWnd, &rect, TRUE );
      UpdateWindow( hWnd );

      // Notify the assoc that the spin button has been released.
      if( StateTest(lpSpin, SPSTATE_SENTNOTIFY) )
        {
        StateClear(lpSpin, SPSTATE_SENTNOTIFY);
        if( bSentDec )
          NotifyAssocWnd( hWnd, lpSpin, SN_DECBUTTONUP, lpSpin->lPos );
        else
          NotifyAssocWnd( hWnd, lpSpin, SN_INCBUTTONUP, lpSpin->lPos );
        }
      }
    }

/****************************************************************************
 * UpdateSpin
 *
 * Purpose:
 *  Called when the spin needs to be repainted.
 *
 * Parameters:
 *  HWND          hWnd         - handle to the spin window
 *  LPRECT        lpRect       - rect to update
 *  BOOL          bEraseBkGrnd - flag to erase background or not
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI UpdateSpin( HWND hWnd, LPRECT lpRect, BOOL bEraseBkGrnd )
    {
    InvalidateRect( hWnd, lpRect, bEraseBkGrnd );
    UpdateWindow( hWnd );
    }

/****************************************************************************
 * ParseSpinText
 *
 * Purpose:
 *  Parses window text for a valid range and initial position.
 *  This function is used when creating the control or setting the
 *  window text to set the initial range and position. It is also used
 *  to validate text entered in the Style dialog in the Dialog Editor.
 *  It is activated by setting the SPS_TEXTHASRANGE style.
 *
 *  The range and position must all be valid for any change to
 *  occur in lpnMin, lpnMax, and lpnPos.
 *
 * Parameters:
 *  LPSTR         lpsz         - pointer to the spin control text
 *  LPLONG        lplMin       - pointer to range minimum
 *  LPLONG        lplMax       - pointer to range maximum
 *  LPLONG        lplPos       - pointer to initial position
 *
 * Return Value:
 *  BOOL          TRUE if the function successfully initializes the values.
 *                FALSE if any part of the text is not a valid number.
 ****************************************************************************/

BOOL WINAPI ParseSpinText( LPSTR lpsz, LPLONG lplMin, LPLONG lplMax, LPLONG lplPos )
    {
    LONG lMin, lMax, lPos;
    BOOL bRet;

    // Return if no string.
    if( !lpsz || !lstrlen(lpsz) )
      return FALSE;

    // Note that we depend on WTranslateUpToChar to modify lpsz to point
    // to the character after the delimiter which is why we pass &lpsz.
    // If we hit an invalid value or the end of the string we abort.

    // Parse for the range minimum.
    bRet = WTranslateUpToChar( &lpsz, &lMin );
    if( !bRet || !*lpsz )
      return FALSE;

    // Parse for the range maximum.
    bRet = WTranslateUpToChar( &lpsz, &lMax );
    if( !bRet || !*lpsz )
      return FALSE;

    // Parse for the initial position.
    bRet = WTranslateUpToChar( &lpsz, &lPos );
    if( !bRet )
      return FALSE;

    // Store the parsed values in the return locations.
    *lplMin = lMin;
    *lplMax = lMax;
    *lplPos = lPos;

    return TRUE;
    }

/****************************************************************************
 * NotifyAssocWnd
 *
 * Purpose:
 *  Sends a notification of an event to the associate window.
 *  In most cases this would be the parent window. If there is a position
 *  value associated with the current notification it is passed in and
 *  temporarily held until the notification is processed.
 *
 * Parameter:
 *  HWND          hWnd        - handle to the spin window
 *  LPSPIN        lpSpin      - pointer to spin struct
 *  UINT          uEvent      - identifier of the event
 *  LONG          lPos        - current position
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI NotifyAssocWnd( HWND hWnd, LPSPIN lpSpin, UINT uEvent, LONG lPos )
    {
    HGLOBAL   hEvent;
    LPSNEVENT lpCNEvent;

    if( lpSpin->dwStyle & SPS_ASYNCNOTIFY )
      {
      hEvent = GlobalAlloc( GHND, LEN_SNEVENT );
      lpCNEvent = (LPSNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lpCNEvent->uCurNotify = uEvent;
        lpCNEvent->lCurPos    = lPos;
        GlobalUnlock( hEvent );
        }

#ifdef WIN16
      FORWARD_WM_COMMAND( lpSpin->hWndAssoc, GetWindowID( hWnd ),
                          hWnd, hEvent, SendMessage );
#else
      // For Win32 we send a custom msg so that Realizer will not stop the msg.
      // If we send a wm_command msg Realizer looks at the hWnd that is supposed
      // to be in the lParam to determine where the msg goes. Since we are passing
      // a memory handle instead of a window handle in the lParam Realizer doesn't
      // know what to do with it and does not forward it to the form proc.
      FORWARD_CCM_CUSTOM( lpSpin->hWndAssoc, GetWindowID( hWnd ),
                          hEvent, uEvent, SendMessage );
#endif
      }
    else
      {
      // Set these in case someone needs to query the values during the
      // notification processing. 
      lpSpin->uCurNotify = uEvent;
      lpSpin->lCurPos    = lPos;

      FORWARD_WM_COMMAND( lpSpin->hWndAssoc, GetWindowID( hWnd ),
                          hWnd, uEvent, SendMessage );

      // Clear the notification message and values.
      lpSpin->uCurNotify = 0;
      lpSpin->lCurPos    = 0L;
      }
    }

/****************************************************************************
 * CATOSPIN.C Helper Functions
 ****************************************************************************/

/****************************************************************************
 * ClickedRectCalc
 *
 * Purpose:
 *  Calculates the rectangle of the clicked region based on the
 *  state flags SPSTATE_UPCLICK, SPSTATE_DOWNCLICK, SPSTATE_LEFTCLICK,
 *  and SPSTATE_RIGHTLICK, depending on the style.
 *
 * Parameter:
 *  HWND          hWnd         - handle to the spin window
 *  LPRECT        lpRect       - rectangle structure to fill.
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR ClickedRectCalc( HWND hWnd, LPSPIN lpSpin, LPRECT lpRect )
    {
    int cx, cy;

    GetClientRect( hWnd, lpRect );
    cx = lpRect->right  >> 1;
    cy = lpRect->bottom >> 1;

    if( lpSpin->dwStyle & SPS_VERTICAL )
      {
      if( StateTest(lpSpin, SPSTATE_DOWNCLICK) )
        lpRect->top=cy;

      if( StateTest(lpSpin, SPSTATE_UPCLICK) )
        lpRect->bottom=1+cy;
      }
    else
      {
      // Come here for SPS_HORIZONTAL button.
      if( StateTest(lpSpin, SPSTATE_RIGHTCLICK) )
        lpRect->left=cx;

      if( StateTest(lpSpin, SPSTATE_LEFTCLICK) )
        lpRect->right=1+cx;
      }

    return;
    }

/****************************************************************************
 * PositionChange
 *
 * Purpose:
 * Determines which part of control has been clicked and then sends 
 * the appropriate notification to the associate window.
 *
 * This function does not send a notification if the position is pegged on
 * the min or max of the range and the SPS_NOPEGNOTIFY style is enabled.
 *
 * Parameter:
 *  HWND          hWnd         - handle to the spin window
 *  LPSPIN        lpSpin      - pointer to spin struct
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI PositionChange( HWND hWnd, LPSPIN lpSpin )
    {
    UINT uScrollCode;
    BOOL bPegged=FALSE;

    if( StateTest(lpSpin, SPSTATE_UPCLICK | SPSTATE_LEFTCLICK) )
      uScrollCode = SN_POSDECREMENT;
    else if( StateTest(lpSpin, SPSTATE_DOWNCLICK | SPSTATE_RIGHTCLICK) )
      uScrollCode = SN_POSINCREMENT;
    else
      return;

   /*-----------------------------------------------------------------------*
    * Modify the current position according to the following rules:
    *
    * 1. On SN_POSDECREMENT with an inverted range, increment the position.
    *    If the position is already at the maximum, set the pegged flag.
    *
    * 2. On SN_POSDECREMENT with an normal range, decrement the position.
    *    If the position is already at the minimum, set the pegged flag.
    *
    * 3. On SN_POSINCREMENT with an inverted range, treat like SPN_DECREMENT
    *    with a normal range.
    *
    * 4. On SN_POSINCREMENT with an normal range, treat like SPN_DECREMENT
    *    with an inverted range.
    *-----------------------------------------------------------------------*/

    if( uScrollCode == SN_POSDECREMENT )
      {
      if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
        {
        if( lpSpin->lPos == lpSpin->lMax )
          bPegged = TRUE;
        else
          lpSpin->lPos++;

        uScrollCode = SN_POSINCREMENT;
        }
      else
        {
        if( lpSpin->lPos == lpSpin->lMin )
          bPegged = TRUE;
        else
          lpSpin->lPos--;
        }
      }
    else
      {
      if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
        {
        if( lpSpin->lPos == lpSpin->lMin )
          bPegged = TRUE;
        else
          lpSpin->lPos--;

        uScrollCode = SN_POSDECREMENT;
        }
      else
        {
        if( lpSpin->lPos == lpSpin->lMax )
          bPegged = TRUE;
        else
          lpSpin->lPos++;
        }
      }

    if( bPegged && (SPS_WRAPMINMAX & lpSpin->dwStyle) )
      {
      if( lpSpin->lPos == lpSpin->lMin )
        {         
        lpSpin->lPos = lpSpin->lMax;
        uScrollCode = SN_POSWRAPTOMAX;
        }
      else
        {         
        lpSpin->lPos = lpSpin->lMin;
        uScrollCode = SN_POSWRAPTOMIN;
        }

      bPegged = FALSE;
      }

    // Send a message if we changed and are not pegged,
    // or did not change and SPS_NOPEGNOTIFY is clear.
    if( !bPegged || !(SPS_NOPEGNOTIFY & lpSpin->dwStyle) )
      {
      StateSet( lpSpin, SPSTATE_SENTNOTIFY );   // set this for the buttonup
      NotifyAssocWnd( hWnd, lpSpin, uScrollCode, lpSpin->lPos );
      }

    return;
    }

/****************************************************************************
 * WTranslateUpToChar
 *
 * Purpose:
 *  Scans a string for digits, converting the series of digits to an 
 *  integer value as the digits are scanned.  Scanning stops at a delimiter
 *  (commas) or the end of the string.
 *
 * Parameters:
 *  LPSTR FAR *   lplpsz      - pointer to pointer to the string to scan.
 *                              On return, the pointer will point to the 
 *                              character after the delimeter OR the 
 *                              NULL terminator.
 *                              We want a pointer to the pointer so we can
 *                              modify that pointer for the calling function
 *                              since we are using the return value for the
 *                              parsed value
 *  LPLONG        lplVal      - pointer to the parsed value
 *
 * Return Value:
 *  BOOL          TRUE  if an integer value was successfully parsed;
 *                FALSE if the string contained non-digits (excluding delimiters)
 ****************************************************************************/

BOOL NEAR WTranslateUpToChar( LPSTR FAR *lplpsz, LPLONG lplVal )
    {
    LPSTR lpsz;
    char  ch;
    LONG  lRet=0;
    BOOL  bNeg=FALSE;

    lpsz = *lplpsz;

    // Fail if there's no more string to parse.
    if( !*lpsz )
      return FALSE;

    // Scan the string until we hit the NULL terminator.
    while( ch=*lpsz )
      {
      // Skip past comma delimiter and end the scan for this token.
      if( ch == ',' )
        {
        lpsz++;  
        break;
        }

      // Remember if this is a negative number.
      if( ch == '-' )
        {
        bNeg = TRUE;
        lpsz++;  
        continue;
        }

      // Check for digits, returning -1 on a non-digit.
      ch -= '0';

      if( ch < 0 || ch > 9 )
        return FALSE;

      // Add up the value as we scan.
      lRet = (lRet*10) + (LONG)ch;

      // We increment lpsz here so if we hit a null-terminator
      // lpsz is always valid.  If we incremented in the while
      // statement then lpsz might be past the null-terminator
      // and possibly invalid.

      lpsz++;
      }

    // Change the sign if this is a negative number.
    if( bNeg )
      lRet = -lRet;

    // Store the new pointer and the value.  Note that the *lpsz++
    // already incremented lpsz past the delimeter AND the zero.

    *lplpsz = lpsz;
    *lplVal = (LONG)lRet;

    return TRUE;
    }
