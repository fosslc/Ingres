/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * SPINAPI.C
 *
 * Contains the API implementation of the Spin button control.
 * The advantage of using a function API instead of a messaging API is
 * that you get type checking on the parameters and the return value.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "spindll.h"

VOID NEAR ChkSpinGrayState( HWND hWnd, LPSPIN lpSpin );

/****************************************************************************
 * SpinGetVersion
 *
 * Purpose:
 *  Gets the version number of the spin button.
 *
 * Parameters:
 *  none
 *
 * Return Value:
 *  Current version: major release number in the hi-order byte,
 *                   minor release number in the lo-order byte.
 ****************************************************************************/

WORD WINAPI SpinGetVersion( void )
    {
    WORD wVersion = SPIN_VERSION;

    return wVersion;
    }

/****************************************************************************
 * SpinAssociateSet
 * SpinAssociateGet
 *
 * Purpose:
 *  Change or retrieve the associate window of the control.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  HWND          hWndAssoc   - handle of new associate
 *
 * Return Value:
 *  HWND          Handle of previous assoc (set) or current assoc (set).
 ****************************************************************************/

HWND WINAPI SpinAssociateSet( HWND hWnd, HWND hWndAssoc )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    HWND   hWndOldAssoc;

    // Save old window handle.
    hWndOldAssoc = lpSpin->hWndAssoc;

    // Let the old assoc know it is no longer the assoc window.
    NotifyAssocWnd( hWnd, lpSpin, SN_ASSOCIATELOSS, 0L );

    // Set the new assoc wnd handle in the struct.
    lpSpin->hWndAssoc = hWndAssoc;

    // Let the new assoc know it is the new assoc window.
    NotifyAssocWnd( hWnd, lpSpin, SN_ASSOCIATEGAIN, 0L );

    // Return the old assoc wnd handle.
    return hWndOldAssoc;
    }

HWND WINAPI SpinAssociateGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->hWndAssoc;
    }

/****************************************************************************
 * SpinStyleSet
 * SpinStyleGet
 * SpinStyleClear
 *
 * Purpose:
 *  Set/Get/Clear styles for the specified spin.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  DWORD         dwStyle     - style flag(s) to set or clear
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI SpinStyleSet( HWND hWnd, DWORD dwStyle )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    DWORD  dwLBstyle;

    // Get the current styles.
    lpSpin->dwStyle = (DWORD) GetWindowLong( hWnd, GWL_STYLE );

    // If trying to set both direction styles, use vertical.
    if( (dwStyle & SPS_VERTICAL) && (dwStyle & SPS_HORIZONTAL) )
      dwStyle &= ~SPS_HORIZONTAL;

    // If setting a direction style, gotta clear the opposite one.
    if( dwStyle & SPS_VERTICAL )
      lpSpin->dwStyle &= ~SPS_HORIZONTAL;
    if( dwStyle & SPS_HORIZONTAL )
      lpSpin->dwStyle &= ~SPS_VERTICAL;

    // If trying to set both nopeg and wrap, use nopeg.
    if( (dwStyle & SPS_NOPEGNOTIFY) && (dwStyle & SPS_WRAPMINMAX) )
      dwStyle &= ~SPS_WRAPMINMAX;

    // If setting no-peg gotta clear the wrap style.
    if( dwStyle & SPS_NOPEGNOTIFY )
      lpSpin->dwStyle &= ~SPS_WRAPMINMAX;

    // If setting wrap gotta clear the no-peg style and any gray state.
    if( dwStyle & SPS_WRAPMINMAX )
      {
      lpSpin->dwStyle &= ~SPS_NOPEGNOTIFY;
      StateClear(lpSpin, SPSTATE_GRAYED);
      }

    // Only set string styles if not set yet (listbox will not exist).
    if( (dwStyle & SPS_HASSTRINGS) || 
        (dwStyle & SPS_HASSORTEDSTRS) && !lpSpin->hWndLB )
      {
      // If trying to set both string styles, use sorted strings.
      if( (dwStyle & SPS_HASSTRINGS) && (dwStyle & SPS_HASSORTEDSTRS) )
        dwStyle &= ~SPS_HASSTRINGS;

      // Remove text-has-range style if using strings.
      lpSpin->dwStyle &= ~SPS_TEXTHASRANGE;

      dwLBstyle = WS_CHILD | LBS_HASSTRINGS;

      // Set the sort flag on if user wants it.
      if( dwStyle & SPS_HASSORTEDSTRS )
        dwLBstyle |= LBS_SORT;

      // Create the invisible listbox to manage the spin button strings.
      lpSpin->hWndLB = CreateWindow( "listbox", NULL, dwLBstyle, 0, 0, 20, 20,
                                      hWnd, (HMENU)IDC_LISTBOX, hInstSpin, 0L );

      if( lpSpin->hWndLB )
        {
        // Reset the range values.
        lpSpin->lMin = 0;
        lpSpin->lMax = 0;
        lpSpin->lPos = 0;
        }
      else
        {
        // If the create failed don't apply the string style.
        dwStyle &= ~(SPS_HASSTRINGS | SPS_HASSORTEDSTRS);
        }
      }

    // Apply the new styles.
    lpSpin->dwStyle |= dwStyle;

    // If no-peg is set chk for any chg in the gray state.
    if( lpSpin->dwStyle & SPS_NOPEGNOTIFY )
      {
      StateClear(lpSpin, SPSTATE_GRAYED);
      
      if( lpSpin->lPos == lpSpin->lMax )
        {
        if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        else
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        }

      if( lpSpin->lPos == lpSpin->lMin )
        {
        if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        else
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        }
      }

    // Reset the window styles.
    SetWindowLong( hWnd, GWL_STYLE, (LONG)lpSpin->dwStyle );

    // Force a repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return;
    }

VOID WINAPI SpinStyleClear( HWND hWnd, DWORD dwStyle )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Get the current styles.
    lpSpin->dwStyle = (DWORD) GetWindowLong( hWnd, GWL_STYLE );

    // Only chk string styles if one is set (listbox will exist).
    if( (dwStyle & SPS_HASSTRINGS) || (dwStyle & SPS_HASSORTEDSTRS) )
      {
      if( lpSpin->hWndLB )
        {
        // Delete the spin button internal listbox.
        DestroyWindow( lpSpin->hWndLB );
        lpSpin->hWndLB = NULL;

        // Reset the range values.
        lpSpin->lMin = 0;
        lpSpin->lMax = 0;
        lpSpin->lPos = 0;
        }
      }

    // Clear the requested styles.
    lpSpin->dwStyle &= ~dwStyle;

    // If clearing no-peg gotta clear any gray state, too.
    if( dwStyle & SPS_NOPEGNOTIFY )
      StateClear(lpSpin, SPSTATE_GRAYED);
      
    // If neither direction styles are set use vertical.
    if( !(lpSpin->dwStyle & SPS_VERTICAL) && !(lpSpin->dwStyle & SPS_HORIZONTAL) )
      lpSpin->dwStyle |= SPS_VERTICAL;

    // If no-peg is still set chk for any chg in the gray state.
    if( lpSpin->dwStyle & SPS_NOPEGNOTIFY )
      {
      StateClear(lpSpin, SPSTATE_GRAYED);
      
      if( lpSpin->lPos == lpSpin->lMax )
        {
        if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        else
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        }

      if( lpSpin->lPos == lpSpin->lMin )
        {
        if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        else
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        }
      }

    // Reset the window styles.
    SetWindowLong( hWnd, GWL_STYLE, (LONG)lpSpin->dwStyle );

    // Force a repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return;
    }

DWORD WINAPI SpinStyleGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->dwStyle;
    }

/****************************************************************************
 * SpinAttribSet
 * SpinAttribGet
 * SpinAttribClear
 *
 * Purpose:
 *  Set/Get/Clear general attributes for the specified spin button.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  DWORD         dwAttrib    - attribute flag(s) to set or clear
 *
 * Return Value:
 *  DWORD         current attribute flags
 ****************************************************************************/

VOID WINAPI SpinAttribSet( HWND hWnd, DWORD dwAttrib )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Set the specified attributes.
    lpSpin->flSpinAttr |= dwAttrib;

    // Force repaint since we changed a state.
    UpdateSpin( hWnd, NULL, TRUE );

    return;
    }

DWORD WINAPI SpinAttribGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->flSpinAttr;
    }

VOID WINAPI SpinAttribClear( HWND hWnd, DWORD dwAttrib )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Clear the specified attributes.
    lpSpin->flSpinAttr &= ~dwAttrib;

    // Force repaint since we changed a state.
    UpdateSpin( hWnd, NULL, TRUE );

    return;
    }

/****************************************************************************
 * SpinCursorSet
 *
 * Purpose:
 *  Sets a special cursor for the spin button.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  HCURSOR       hCursor     - handle to the new cursor
 *
 * Return Value:
 *  BOOL          TRUE if curson was set successfully; else FALSE
 ****************************************************************************/

BOOL WINAPI SpinCursorSet( HWND hWnd, HCURSOR hCursor )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    lpSpin->hCursor = hCursor;

    return TRUE;
    }

/****************************************************************************
 * SpinColorSet
 * SpinColorGet
 *
 * Purpose:
 *  Change or retrieve a configurable color.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  UINT          iColor      - index to the control to modify or retrieve
 *  COLORREF      cr          - new value of the color
 *
 * Return Value:
 *  COLORREF      Previous (set) or current (get) color value.
 ****************************************************************************/

COLORREF WINAPI SpinColorSet( HWND hWnd, UINT iColor, COLORREF cr )
    {
    LPSPIN   lpSpin = GETSPIN(hWnd);
    COLORREF crOld;

    if( iColor >= SPINCOLORS )
      return 0L;

    // Save the old color.
    crOld = lpSpin->crSpin[iColor];

    // Set the new color.
    lpSpin->crSpin[iColor] = cr;

    // Update the spin colors.
    SpinColorManager( hWnd, lpSpin, CLR_UPDATE );

    // Force repaint since we changed a state.
    UpdateSpin( hWnd, NULL, TRUE );

    // Return the old color.
    return crOld;
    }

COLORREF WINAPI SpinColorGet( HWND hWnd, UINT iColor )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    if( iColor >= SPINCOLORS )
      return 0L;

    return lpSpin->crSpin[iColor];
    }

/****************************************************************************
 * SpinRangeSet
 *
 * Purpose:
 *  Set the scrolling range of the spin button.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  LONG          lMin        - minimum position of the scrolling range
 *  LONG          lMax        - maximum position of the scrolling range
 *
 * Return Value:
 *  BOOL          TRUE if range was chged successfully; else FALSE
 ****************************************************************************/

BOOL WINAPI SpinRangeSet( HWND hWnd, LONG lMin, LONG lMax )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    LONG   lMinOld, lMaxOld;
    BOOL   bPosChg=FALSE;

    // This function is ignored if strings are enabled.
    if( lpSpin->hWndLB )
      return FALSE;

    // Set the new range, sending the appropriate notifications.
    // Also send a scroll message if the position has to change.
    // If the minimum is greater than the max, return error.
    if( lMin > lMax )
      return FALSE;

    // Save old values.
    lMinOld = lpSpin->lMin;
    lMaxOld = lpSpin->lMax;

    // Set the new values.
    lpSpin->lMin = lMin;
    lpSpin->lMax = lMax;

    // If current position is outside of new range, force it to
    // one of the boundaries, otherwise leave it be.
    if( (lpSpin->lMin > lpSpin->lPos) || (lpSpin->lMax < lpSpin->lPos) )
      {
      lpSpin->lPos = ((lpSpin->lMin > lpSpin->lPos) ? lpSpin->lMin : lpSpin->lMax);
      bPosChg = TRUE;
      }

    // Notify the assoc of the range and position (maybe) change.  
    NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
    if( bPosChg )
      NotifyAssocWnd( hWnd, lpSpin, SN_POSCHANGE, lpSpin->lPos );

    return TRUE;
    }

/****************************************************************************
 * SpinRangeMinGet
 * SpinRangeMaxGet
 *
 * Purpose:
 *  Get the minimum or maximum range position.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *
 * Return Value:
 *  LONG          range minimum or maximum position
 ****************************************************************************/

LONG WINAPI SpinRangeMinGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->lMin;
    }

LONG WINAPI SpinRangeMaxGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->lMax;
    }

/****************************************************************************
 * SpinPositionSet
 * SpinPositionGet
 *
 * Purpose:
 *  Set/Get the current position of the control.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *  LONG          lPos        - new position to set
 *
 * Return Value:
 *  LONG          Previous (set) or current (get) position.
 ****************************************************************************/

LONG WINAPI SpinPositionSet( HWND hWnd, LONG lPos )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    LONG   lOldPos=0;
    BOOL   bUpdate=FALSE;

    // If lPos is less than the minimum set it to the minimum.
    // If lPos is greater than the maximum set it to the maximum.
    if( lPos < lpSpin->lMin )
      lPos = lpSpin->lMin;
    if( lPos > lpSpin->lMax )
      lPos = lpSpin->lMax;

    // If already there don't do anything.
    if( lPos == lpSpin->lPos )
      return lPos;

    // Save current position.
    lOldPos = lpSpin->lPos;

    // Set the new position.
    lpSpin->lPos = lPos;

    // Gray or ungray the button halves.
    ChkSpinGrayState( hWnd, lpSpin );

    // Tell assoc of the position change.
    NotifyAssocWnd( hWnd, lpSpin, SN_POSCHANGE, lpSpin->lPos );

    // Return the old position.
    return lOldPos;
    }

LONG WINAPI SpinPositionGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    return lpSpin->lPos;
    }

/****************************************************************************
 * SpinPositionInc
 * SpinPositionDec
 *
 * Purpose:
 *  Increment or decrement the position of the spin button.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control window
 *
 * Return Value:
 *  LONG          new position after increment or decrement
 ****************************************************************************/

LONG WINAPI SpinPositionInc( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    UINT   uState;

    // Decide which button half is incrementing.
    if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
      uState = SPSTATE_LEFTCLICK;
    else
      uState = SPSTATE_RIGHTCLICK;

    // Increase the scrolling range by one.
    StateSet(lpSpin, uState);
    PositionChange( hWnd, lpSpin );
    StateClear(lpSpin, uState);

    // Gray or ungray the button halves.
    ChkSpinGrayState( hWnd, lpSpin );

    return lpSpin->lPos;
    }

LONG WINAPI SpinPositionDec( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    UINT   uState;

    // Decide which button half is decrementing.
    if( lpSpin->dwStyle & SPS_INVERTCHGBTN )
      uState = SPSTATE_RIGHTCLICK;
    else
      uState = SPSTATE_LEFTCLICK;

    // Decrease the scrolling range by one.
    StateSet(lpSpin, uState);
    PositionChange( hWnd, lpSpin );
    StateClear(lpSpin, uState);

    // Gray or ungray the button halves.
    ChkSpinGrayState( hWnd, lpSpin );

    return lpSpin->lPos;
    }

/****************************************************************************
 * SpinTopBmpSet
 * SpinBottomBmpSet
 * SpinLeftBmpSet
 * SpinRightBmpSet
 *
 * Purpose:
 *  Sets a custom bitmap that will be used to draw the direction indicators
 *  on the face of either button half. Top and bottom should be used with a
 *  vertical (SPS_VERTICAL) spin button and left and right should be used
 *  with a horizontal (SPS_HORIZONTAL) spin button.
 *  The bitmap can be specified as having a transparent background by
 *  turning on the bTransparent flag and specifying the X/Y position of a
 *  pixel in the bitmap that contains the background color.
 *  It can also have precision placement by specifying an X or Y offset
 *  (in pixels) which is applied after centering the bitmap on the button.
 *
 * Parameters:
 *  HWND          hWnd          - Spin button window handle 
 *  HBITMAP       hBmp          - bitmap to be used for drawing the indicator
 *  int           xOffBmp       - horz offset for bitmap placement
 *  int           yOffBmp       - vert offset for bitmap placement
 *  int           xTransPxl     - xpos of transparent bkgrnd pixel for bitmap 
 *  int           yTransPxl     - ypos of transparent bkgrnd pixel for bitmap 
 *  BOOL          bTransparent  - if TRUE bitmap background is transparent
 *  BOOL          bStretchToFit - if TRUE bitmap is stretched or compressed 
 *                                (not supported yet)
 * Return Value:
 *  BOOL          TRUE: success;  FALSE: error
 ****************************************************************************/

BOOL WINAPI SpinTopBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp,
                           int yOffBmp, int xTransPxl, int yTransPxl, 
                           BOOL bTransparent, BOOL bStretchToFit )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Set the bitmap info into the struct.
    lpSpin->hBmpDec       = hBmp;
    lpSpin->xOffBmpDec    = xOffBmp;
    lpSpin->yOffBmpDec    = yOffBmp;
    lpSpin->xTransPxlDec  = xTransPxl;
    lpSpin->yTransPxlDec  = yTransPxl;
    lpSpin->bDecBmpTrans  = bTransparent;

    // Force repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return TRUE;
    }

BOOL WINAPI SpinBottomBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp,
                              int yOffBmp, int xTransPxl, int yTransPxl, 
                              BOOL bTransparent, BOOL bStretchToFit )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Set the bitmap info into the struct.
    lpSpin->hBmpInc       = hBmp;
    lpSpin->xOffBmpInc    = xOffBmp;
    lpSpin->yOffBmpInc    = yOffBmp;
    lpSpin->xTransPxlInc  = xTransPxl;
    lpSpin->yTransPxlInc  = yTransPxl;
    lpSpin->bIncBmpTrans  = bTransparent;

    // Force repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return TRUE;
    }

BOOL WINAPI SpinLeftBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp,
                            int yOffBmp, int xTransPxl, int yTransPxl, 
                            BOOL bTransparent, BOOL bStretchToFit )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Set the bitmap info into the struct.
    lpSpin->hBmpDec       = hBmp;
    lpSpin->xOffBmpDec    = xOffBmp;
    lpSpin->yOffBmpDec    = yOffBmp;
    lpSpin->xTransPxlDec  = xTransPxl;
    lpSpin->yTransPxlDec  = yTransPxl;
    lpSpin->bDecBmpTrans  = bTransparent;

    // Force repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return TRUE;
    }

BOOL WINAPI SpinRightBmpSet( HWND hWnd, HBITMAP hBmp, int xOffBmp,
                             int yOffBmp, int xTransPxl, int yTransPxl, 
                             BOOL bTransparent, BOOL bStretchToFit )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Set the bitmap info into the struct.
    lpSpin->hBmpInc       = hBmp;
    lpSpin->xOffBmpInc    = xOffBmp;
    lpSpin->yOffBmpInc    = yOffBmp;
    lpSpin->xTransPxlInc  = xTransPxl;
    lpSpin->yTransPxlInc  = yTransPxl;
    lpSpin->bIncBmpTrans  = bTransparent;

    // Force repaint.
    UpdateSpin( hWnd, NULL, TRUE );

    return TRUE;
    }

/****************************************************************************
 * SpinNotifyAssoc
 *
 * Purpose:
 *  Allows an application to pipe a notification message thru the spin button
 *  to its associate window. In most cases this would be the parent window.
 *  If there is a position associated with the this notification it is passed
 *  in and temporarily held until the notification is processed.
 *
 * Parameter:
 *  HWND          hWnd        - Spin button window handle 
 *  UINT          uEvent      - identifier of the event
 *  LONG          lPos        - current position
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI SpinNotifyAssoc( HWND hWnd, UINT uEvent, LONG lPos )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    // Pipe the message to the associate window.
    NotifyAssocWnd( hWnd, lpSpin, uEvent, lPos );
    }

/****************************************************************************
 * SpinNotifyGet
 *
 * Purpose:
 *  Retrieve the current spin button notification value.
 *  This procedure only needs to be used for spin buttons using the
 *  asynchronous notification method (SPS_ASYNCNOTIFY) and should be
 *  called immediately upon receiving a notification message.
 *  
 * Parameters:
 *  HWND          hWnd        - Spin button window handle 
 *  LPARAM        lParam      - lparam of a spin button notification msg
 *                              (ignored if SPS_ASYNCNOTIFY is not enabled)
 * Return Value:
 *  UINT          current notification value
 ****************************************************************************/

UINT WINAPI SpinNotifyGet( HWND hWnd, LPARAM lParam )
    {
    LPSPIN    lpSpin = GETSPIN(hWnd);
#ifdef WIN16
    HGLOBAL   hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL   hEvent = (HGLOBAL)lParam;
#endif
    LPSNEVENT lpSNEvent;
    UINT      uCurNotify;

    if( (lpSpin->dwStyle & SPS_ASYNCNOTIFY) && hEvent )
      {
      lpSNEvent = (LPSNEVENT) GlobalLock( hEvent );
      if( lpSNEvent )
        {
        uCurNotify = lpSNEvent->uCurNotify;
        GlobalUnlock( hEvent );
        return uCurNotify;
        }
      }
    else
      {
      return lpSpin->uCurNotify;
      }
    }

/****************************************************************************
 * SpinNotifyDone
 *
 * Purpose:
 *  Release memory for an asynchronous spin button notification message.
 *  This procedure only needs to be used for spin buttons using the
 *  asynchronous notification method (SPS_ASYNCNOTIFY) and should be
 *  called immediately after processing the notification.
 *
 * Parameters:
 *  HWND          hWnd        - Spin button window handle 
 *  LPARAM        lParam      - lParam of a spin button notification msg
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI SpinNotifyDone( HWND hWnd, LPARAM lParam )
    {
    LPSPIN  lpSpin = GETSPIN(hWnd);
#ifdef WIN16
    HGLOBAL hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL hEvent = (HGLOBAL)lParam;
#endif

    if( (lpSpin->dwStyle & SPS_ASYNCNOTIFY) && hEvent )
      {    
      GlobalUnlock( hEvent );
      GlobalFree( hEvent );
      }
    return;
    }

/****************************************************************************
 * SpinSNPosGet
 *
 * Purpose:
 *  Retrieve the current position associated with the current notification
 *  being processed by the associate window.
 *
 *  This value is valid ONLY during the processing of the notification.
 *
 * Parameters:
 *  HWND          hWnd        - Spin button window handle 
 *  LPARAM        lParam      - lparam of a spin button notification msg
 *                              (ignored if SPS_ASYNCNOTIFY is not enabled)
 *
 * Return Value:
 *  LONG          current position
 ****************************************************************************/

LONG WINAPI SpinSNPosGet( HWND hWnd, LPARAM lParam )
    {
    LPSPIN    lpSpin = GETSPIN(hWnd);
#ifdef WIN16
    HGLOBAL   hEvent = (HGLOBAL)HIWORD(lParam);
#else
    HGLOBAL   hEvent = (HGLOBAL)lParam;
#endif
    LPSNEVENT lpSNEvent;
    LONG      lPos;
 
    if( (lpSpin->dwStyle & SPS_ASYNCNOTIFY) && hEvent )
      {
      lpSNEvent = (LPSNEVENT) GlobalLock( hEvent );
      if( lpSNEvent )
        {
        lPos = lpSNEvent->lCurPos;
        GlobalUnlock( hEvent );
        return lPos;
        }  
      }  
    else
      {
      return lpSpin->lCurPos;
      }
    }


/****************************************************************************
 * SpinStrAdd
 * SpinStrDelete
 * SpinStrInsert
 * SpinStrFileNameAdd
 * SpinStrPrefixFind
 * SpinStrExactFind
 * SpinStrCountGet
 * SpinStrGet
 * SpinStrLenGet
 * SpinStrDataSet
 * SpinStrDataGet
 * SpinStrContentReset
 *
 * Purpose:
 *  Spin button string manipulation functions. These functions are only
 *  valid if SPS_HASSTRINGS or SPS_HASSORTEDSTRS is enabled.
 *  These functions manipulate the internal listbox by sending the
 *  following messages:
 *
 *     SpinStrAdd           -  LB_ADDSTRING
 *     SpinStrDelete        -  LB_INSERTSTRING
 *     SpinStrInsert        -  LB_DELETESTRING
 *     SpinStrFileNameAdd   -  LB_DIR
 *     SpinStrPrefixFind    -  LB_FINDSTRING
 *     SpinStrExactFind     -  LB_FINDSTRINGEXACT
 *     SpinStrCountGet      -  LB_GETCOUNT
 *     SpinStrGet           -  LB_GETTEXT
 *     SpinStrLenGet        -  LB_GETTEXTLEN
 *     SpinStrDataSet       -  LB_ITEMDATASET
 *     SpinStrDataGet       -  LB_ITEMDATAGET
 *     SpinStrContentReset  -  LB_RESETCONTENT
 *
 *  The documentation for each function should be identical to that
 *  listed in the Windows SDK for the corresponding listbox message.
 *
 * Parameters:
 *  HWND          hWnd        - Spin button window handle 
 *  int           nIndex      - zero-based index
 *  LPSTR         lpszString  - input/output buffer for the string
 *
 * Return Value:
 *  int           meaning varies with the function
 ****************************************************************************/

int WINAPI SpinStrAdd( HWND hWnd, LPSTR lpszString )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB || !lpszString )
      return SPNSTR_ERR;

    nRet = ListBox_AddString( lpSpin->hWndLB, lpszString );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;
    else if( nRet == LB_ERRSPACE )
      nRet = SPNSTR_ERRSPACE;
    else
      {
      // Adjust the range maximum.
      lpSpin->lMax = ListBox_GetCount( lpSpin->hWndLB ) - 1;

      ChkSpinGrayState( hWnd, lpSpin );

      NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
      }

    return nRet;
    }

int WINAPI SpinStrDelete( HWND hWnd, int nIndex )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    BOOL   bPosChg=FALSE;
    int    nRet;

    if( !lpSpin->hWndLB )
      return SPNSTR_ERR;

    nRet = ListBox_DeleteString( lpSpin->hWndLB, nIndex );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;
    else
      {
      // Adjust the range maximum.
      lpSpin->lMax = max(0, nRet-1);

      // Adjust the position if it was sitting on the max.
      if( lpSpin->lPos > lpSpin->lMax )
        {
        lpSpin->lPos = lpSpin->lMax;
        bPosChg = TRUE;
        }

      ChkSpinGrayState( hWnd, lpSpin );

      NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
      if( bPosChg )
        NotifyAssocWnd( hWnd, lpSpin, SN_POSCHANGE, lpSpin->lPos );
      }

    return nRet;
    }

int WINAPI SpinStrInsert( HWND hWnd, int nIndex, LPSTR lpszString )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB || !lpszString )
      return SPNSTR_ERR;

    nRet = ListBox_InsertString( lpSpin->hWndLB, nIndex, lpszString );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;
    else if( nRet == LB_ERRSPACE )
      nRet = SPNSTR_ERRSPACE;
    else
      {
      // Adjust the range maximum.
      lpSpin->lMax = ListBox_GetCount( lpSpin->hWndLB ) - 1;

      ChkSpinGrayState( hWnd, lpSpin );

      NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
      }

    return nRet;
    }

int WINAPI SpinStrFileNameAdd( HWND hWnd, UINT uAttrs, LPSTR lpszFileSpec )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;
    
    if( !lpSpin->hWndLB || !lpszFileSpec )
      return SPNSTR_ERR;

    nRet = ListBox_Dir( lpSpin->hWndLB, uAttrs, lpszFileSpec );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;
    else if( nRet == LB_ERRSPACE )
      nRet = SPNSTR_ERRSPACE;
    else
      {
      // Adjust the range maximum.
      lpSpin->lMax = ListBox_GetCount( lpSpin->hWndLB ) - 1;

      ChkSpinGrayState( hWnd, lpSpin );

      NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
      }

    return nRet;
    }

int WINAPI SpinStrPrefixFind( HWND hWnd, int nIndexStart, LPSTR lpszPrefix )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB || !lpszPrefix )
      return SPNSTR_ERR;

    nRet = ListBox_FindString( lpSpin->hWndLB, nIndexStart, lpszPrefix );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

int WINAPI SpinStrExactFind( HWND hWnd, int nIndexStart, LPSTR lpszString )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB || !lpszString )
      return SPNSTR_ERR;

    nRet = ListBox_FindStringExact( lpSpin->hWndLB, nIndexStart, lpszString );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

int WINAPI SpinStrCountGet( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB )
      return SPNSTR_ERR;

    nRet = ListBox_GetCount( lpSpin->hWndLB );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

int WINAPI SpinStrGet( HWND hWnd, int nIndex, LPSTR lpszString )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB || !lpszString )
      return SPNSTR_ERR;

    lpszString[0] = 0;

    nRet = ListBox_GetText( lpSpin->hWndLB, nIndex, lpszString );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

int WINAPI SpinStrLenGet( HWND hWnd, int nIndex )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB )
      return SPNSTR_ERR;

    nRet = ListBox_GetTextLen( lpSpin->hWndLB, nIndex );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

int WINAPI SpinStrDataSet( HWND hWnd, int nIndex, LONG lData )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    int    nRet;

    if( !lpSpin->hWndLB )
      return SPNSTR_ERR;

    nRet = ListBox_SetItemData( lpSpin->hWndLB, nIndex, lData );

    if( nRet == LB_ERR )
      nRet = SPNSTR_ERR;

    return nRet;
    }

LONG WINAPI SpinStrDataGet( HWND hWnd, int nIndex )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);
    LONG   lRet;

    if( !lpSpin->hWndLB )
      return SPNSTR_ERR;

    lRet = ListBox_GetItemData( lpSpin->hWndLB, nIndex );

    if( lRet == LB_ERR )
      lRet = SPNSTR_ERR;

    return lRet;
    }

VOID WINAPI SpinStrContentReset( HWND hWnd )
    {
    LPSPIN lpSpin = GETSPIN(hWnd);

    if( lpSpin->hWndLB )
      {
      ListBox_ResetContent( lpSpin->hWndLB );

      // Adjust the range maximum and position.
      lpSpin->lMax = 0;
      lpSpin->lPos = 0;

      ChkSpinGrayState( hWnd, lpSpin );

      NotifyAssocWnd( hWnd, lpSpin, SN_RANGECHANGE, 0L );
      }

    return;
    }

/****************************************************************************
 * ChkSpinGrayState
 *
 * Purpose:
 *  Needs to be called anytime a string is added or removed from the
 *  spin button.
 *
 * Parameters:
 *  HWND          hWnd        - Spin button window handle 
 *  LPSPIN        lpSpin      - spin struct
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR ChkSpinGrayState( HWND hWnd, LPSPIN lpSpin )
    {
    BOOL bUpdate=FALSE;

    // See if we have to gray or ungray either button half.
    if( SPS_NOPEGNOTIFY & lpSpin->dwStyle )
      {
      if( StateTest(lpSpin, SPSTATE_GRAYED) )
        {
        StateClear(lpSpin, SPSTATE_GRAYED);
        bUpdate = TRUE;
        }
      
      if( lpSpin->lPos == lpSpin->lMax )
        {
        if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        else
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        bUpdate = TRUE;
        }

      if( lpSpin->lPos == lpSpin->lMin )
        {
        if( SPS_INVERTCHGBTN & lpSpin->dwStyle )
          StateSet(lpSpin, SPSTATE_DOWNGRAYED);
        else
          StateSet(lpSpin, SPSTATE_UPGRAYED);
        bUpdate = TRUE;
        }

      // Force a repaint if a button half got grayed or ungrayed.
      if( bUpdate )
        UpdateSpin( hWnd, NULL, TRUE );
      }

    return;
    }
