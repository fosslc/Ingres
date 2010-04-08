/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CNTSPLIT.C
 *
 * Contains the procedures for manipulating container split bars.
 ****************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "cntdll.h"

/****************************************************************************
 * SplitBarManager
 *
 * Purpose:
 *  Main proc for handling split bar dragging, creating, moving, and deleting.
 *
 * Parameters:
 *  HWND          hWndFrame   - handle to the container frame window
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  UINT          wEvent      - type of split bar action to execute
 *  LPPOINT       lpPt        - point (in FRAME CLIENT coords) to start event
 *
 * Return Value:
 *  BOOL          TRUE  if event was completed with no error
 *                FALSE if event could not be completed
 ****************************************************************************/

BOOL WINAPI SplitBarManager( HWND hWndFrame, LPCONTAINER lpCnt, UINT wEvent, LPPOINT lpPt )
    {
    int     nHeight, nWidth;
    POINT   ptStart, ptEnd;
    LPSPLIT lpSplitBar;
    RECT    rect;
    BOOL    bSuccess=FALSE;

    if( !lpPt )
      return bSuccess;

    // Get extents of Frame Window Client Area
    GetClientRect( hWndFrame, &rect );
    nWidth  = rect.right;
    nHeight = rect.bottom;

    // Account for a vertical scroll bar if it is showing.
    if( lpCnt->dwStyle & CTS_VERTSCROLL && 
        lpCnt->lpCI->dwRecordNbr > (DWORD)lpCnt->lpCI->nRecsDisplayed-1 )
      rect.right -= lpCnt->nMinWndWidth;

    ptStart.x = rect.left;
    ptStart.y = rect.top;

    // Cursor positioning requires screen coordinates.
    ClientToScreen( hWndFrame, &ptStart );

    if( wEvent == SPLITBAR_CREATE )
      {
      // If already split don't create another until multiple split bars.
      if( !lpCnt->bIsSplit )
        {
        // Start dragging the split bar from the passed in point.
        ClientToScreen( hWndFrame, lpPt );
        SetCursorPos( lpPt->x, lpPt->y );

        GetCursorPos( &ptStart );
        ScreenToClient( hWndFrame, &ptStart );

        // Create the new split bar.
        if( DragSplitBar( hWndFrame, lpCnt, &rect, &ptStart, &ptEnd ) )
          {
          CreateSplitBar( hWndFrame, lpCnt, ptEnd.x );
          bSuccess = TRUE;
          }
        }
      }
    else if( wEvent == SPLITBAR_MOVE )
      {
      // User wants to move an existing split bar so find the split
      // bar in the list and move the cursor to the center of it.
      if( lpSplitBar = GetSplitBar( lpCnt, lpPt->x ) )
        {
        ClientToScreen( hWndFrame, lpPt );
        ptStart.x += lpSplitBar->xSplitStr + lpCnt->nHalfBarWidth;
        ptStart.y  = lpPt->y;
        SetCursorPos( ptStart.x, ptStart.y );
        ScreenToClient( hWndFrame, &ptStart );

        // Move this split bar to new user specified location.
        if( DragSplitBar( hWndFrame, lpCnt, &rect, &ptStart, &ptEnd ) )
          {
          MoveSplitBar( lpCnt, ptStart.x, ptEnd.x );
          bSuccess = TRUE;
          }
        }
      }
    else if( wEvent == SPLITBAR_DELETE )
      {
      DeleteSplitBar( lpCnt, lpPt->x );
      bSuccess = TRUE;
      }

    return bSuccess;
    }

/****************************************************************************
 * DragSplitBar
 *
 * Purpose:
 *  Displays and manages the dragging split bar and returns the mouse point
 *  where the dragging ended. 
 *
 *  NOTE: This does not actually split a child. 
 *
 * Parameters:
 *  HWND          hWndFrame   - handle to the container frame window
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  RECT          lpRect      - constraining rectangle for dragging the split bar
 *  LPPOINT       lpPtStart   - point to start dragging the split bar
 *  LPPOINT       lpPtEnd     - returned point where dragging ended
 *
 * Return Value:
 *  BOOL          TRUE  if user did not abort split bar drag
 *                FALSE if user aborted split bar drag
 ****************************************************************************/

BOOL WINAPI DragSplitBar( HWND hWndFrame, LPCONTAINER lpCnt, LPRECT lpRect, LPPOINT lpPtStart, LPPOINT lpPtEnd )
    {
    HDC  hDC;
    MSG  msgModal;          // Message received during split-bar drag
    BOOL bSplit = TRUE;     // return is TRUE if user ends up inside the frame
    BOOL bTracking = TRUE;  // Is the split-bar drag being tracked?
    static POINT pt;        // Co-ordinates of cursor when split-bar is dragged
    HCURSOR hOldCursor;
    RECT rect;
    int  nHeight;


    hDC = GetDC(hWndFrame);

    // Set up a constraining rect for the cursor.
    pt.x = lpRect->left;
    pt.y = lpRect->top;
    ClientToScreen( hWndFrame, &pt );
    rect.left = pt.x + lpCnt->nHalfBarWidth;
    rect.top  = pt.y;
    pt.x = lpRect->right;
    pt.y = lpRect->bottom;
    ClientToScreen( hWndFrame, &pt );
    rect.right  = pt.x;
    rect.bottom = pt.y;

    ClipCursor( &rect );
    SetCapture( hWndFrame );
    hOldCursor = SetCursor( lpCnt->hCurSplitBar );

    // Set height of dragging area.
    nHeight = lpRect->bottom;

    // Get the starting point for dragging the split bar.
    pt.x = lpPtStart->x;
    pt.y = lpPtStart->y;

    // Draw initial split-bar
    PatBlt( hDC, pt.x - lpCnt->nHalfBarWidth, lpRect->top, lpCnt->nBarWidth, nHeight, DSTINVERT );

    // While the split-bar is being dragged get mouse msgs.
    while( bTracking )
      {
      while( !PeekMessage( &msgModal, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ) )
        WaitMessage();

      switch( msgModal.message )
        {
        case WM_MOUSEMOVE:
          // Erase previous split-bar.
          PatBlt( hDC, pt.x - lpCnt->nHalfBarWidth, lpRect->top, lpCnt->nBarWidth, nHeight, DSTINVERT );

          // Get new mouse position and draw new split bar.
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
          PatBlt( hDC, pt.x - lpCnt->nHalfBarWidth, lpRect->top, lpCnt->nBarWidth, nHeight, DSTINVERT );
          break;

        case WM_LBUTTONUP:              // End of split-bar drag          
          // Erase the last split-bar 
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
          PatBlt( hDC, pt.x - lpCnt->nHalfBarWidth, lpRect->top, lpCnt->nBarWidth, nHeight, DSTINVERT );
          bTracking = FALSE;  // Break out of tracking loop
          break;

        case WM_RBUTTONDOWN:            // Abort split-bar drag          
          // Erase the last split-bar 
          pt.x = LOWORD(msgModal.lParam);
          pt.y = HIWORD(msgModal.lParam);
          PatBlt( hDC, pt.x - lpCnt->nHalfBarWidth, lpRect->top, lpCnt->nBarWidth, nHeight, DSTINVERT );
          bTracking = FALSE;  // Break out of tracking loop

          bSplit = FALSE;      // Set the abort flag.
          break;
        }
      }

    ReleaseCapture();
    ClipCursor( NULL );
    SetCursor( hOldCursor );

    // Return the point where the split bar ended.
    lpPtEnd->x = pt.x;
    lpPtEnd->y = pt.y;

    ReleaseDC( hWndFrame, hDC );
    return bSplit;
    }

/****************************************************************************
 * CreateSplitBar
 *
 * Purpose:
 *  Creates a split bar at xSplit by dividing whatever child is located
 *  at that X coord into 2 windows. The new window will be to the left
 *  of the existing window and will never have a vertical scroll bar.
 *
 * Parameters:
 *  HWND          hWndFrame   - handle to the container frame window
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xSplit      - X coord where a child window will be split
 *
 * Return Value:
 *  HWND          handle to new child window
 ****************************************************************************/

HWND WINAPI CreateSplitBar( HWND hWndFrame, LPCONTAINER lpCnt, int xSplit )
    {
    HWND  hWndChild;
    HWND  hWndLeft;
    RECT  rect;
    DWORD dwStyle;
    int   xLeft, cxLeft, xRight, cxRight;
    int   xOrg, cxTot, xNewSplit;
    int   yChild, cyChild;

    // Get the child located at this xCoord.
    hWndChild = GetChildAtPt( hWndFrame, lpCnt, xSplit );

    if( !hWndChild )
      return NULL;

    // Use the same styles for the new split window.
    dwStyle = (DWORD) GetWindowLong( hWndChild, GWL_STYLE );

    // Get the origin and extents of the child being split. The old X
    // origin will be origin of left wnd and the old width will be the
    // total of the 2 new ones. The Y origin and height never change.
    GetChildPos( hWndChild, &xOrg, &yChild, &cxTot, &cyChild );

    // Flag that we are creating a split child.
    StateSet( lpCnt, CNTSTATE_CREATINGSB );

    hWndLeft = CreateWindow( CNTCHILD_CLASS, "", dwStyle,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hWndFrame, NULL, hInstCnt, (LPVOID)lpCnt );
    if( hWndLeft )
      {
      // Left child never has a vert scroll bar.
      SetWindowLong( hWndLeft, GWL_SHOWVSCROLL, FALSE );
     
      // Store new split Xcoord and also remember it as a fraction of
      // the frame width so that split windows can be proportionally
      // resized when the frame is resized.     
      GetClientRect( hWndFrame, &rect );

      // Account for a vertical scroll bar if it is showing.
      if( lpCnt->dwStyle & CTS_VERTSCROLL && 
          lpCnt->lpCI->dwRecordNbr > (DWORD)lpCnt->lpCI->nRecsDisplayed-1 )
        rect.right -= lpCnt->nMinWndWidth;

      // Make sure it is within the frame client area.
      xSplit = max(lpCnt->nHalfBarWidth, min(xSplit, rect.right-lpCnt->nHalfBarWidth+1));

      xNewSplit = xSplit - lpCnt->nHalfBarWidth;

      // Store the split bar info in the list (currently a list of one).
      lpCnt->SplitBar.xSplitStr     = xNewSplit;  
      lpCnt->SplitBar.xSplitEnd     = xNewSplit + lpCnt->nBarWidth - 1;
      lpCnt->SplitBar.fSplitPerCent = (float)xNewSplit / (float)rect.right;
      lpCnt->SplitBar.hWndLeft      = hWndLeft;
      lpCnt->SplitBar.hWndRight     = hWndChild;

      lpCnt->bIsSplit = TRUE;

      // Size the windows leaving space for the split between them.
      xLeft  = xOrg;
      cxLeft = xNewSplit - xOrg;
      MoveWindow( hWndLeft, xLeft, yChild, cxLeft, cyChild, FALSE );

      xRight  = xNewSplit + lpCnt->nBarWidth;
      cxRight = cxTot - (cxLeft + lpCnt->nBarWidth);
      MoveWindow( hWndChild, xRight, yChild, cxRight, cyChild, FALSE );

      InvalidateRect( hWndFrame, NULL, TRUE );
      UpdateWindow( hWndFrame );

      // Notify the assoc wnd of the new split bar.
      NotifyAssocWnd( hWndChild, lpCnt, CN_SPLITBAR_CREATED, 0, NULL, NULL, xSplit, 0, 0, NULL );
      }

    return hWndLeft;
    }

/****************************************************************************
 * MoveSplitBar
 *
 * Purpose:
 *  Moves an existing split bar between 2 windows by moving the inside
 *  borders touching the split bar to the new X coord at xNewSplit.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure
 *  int           xOldSplit   - X coord of the split bar before being moved
 *  int           xMovSplit   - X coord of the split bar after being moved
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI MoveSplitBar( LPCONTAINER lpCnt, int xOldSplit, int xMovSplit )
    {
    HWND  hWndFrame, hWndLeft, hWndRight;
    RECT  rect;
    LPSPLIT lpSplitBar;
    int   xLeft, cxLeft, xRight, cxRight, cxTot;
    int   yLeft, cyLeft, yRight, cyRight;
    int   xNewSplit;

    if( lpSplitBar = GetSplitBar( lpCnt, xOldSplit ) )
      {
      hWndLeft  = lpSplitBar->hWndLeft;
      hWndRight = lpSplitBar->hWndRight;
      }
    else
      {
      return;
      }

    hWndFrame = GetParent( hWndLeft );   // Get the frame wnd handle.

    // Store new split Xcoord and also remember it as a fraction of
    // the frame width so that split windows can be proportionally
    // resized when the frame is resized.     
    GetClientRect( hWndFrame, &rect );

    // Account for a vertical scroll bar if it is showing.
    if( lpCnt->dwStyle & CTS_VERTSCROLL && 
        lpCnt->lpCI->dwRecordNbr > (DWORD)lpCnt->lpCI->nRecsDisplayed-1 )
      rect.right -= lpCnt->nMinWndWidth;

    // Make sure it is within the frame client area.
    xMovSplit = max(lpCnt->nHalfBarWidth, min(xMovSplit, rect.right-lpCnt->nHalfBarWidth+1));

    xNewSplit = xMovSplit - lpCnt->nHalfBarWidth;

    // Update the split bar list.
    lpSplitBar->xSplitStr     = xNewSplit;                
    lpSplitBar->xSplitEnd     = xNewSplit + lpCnt->nBarWidth - 1;
    lpSplitBar->fSplitPerCent = (float)xNewSplit / (float)rect.right;                

    // Get the origin and extents of the left and right wnds.
    GetChildPos( hWndLeft,  &xLeft,  &yLeft,  &cxLeft,  &cyLeft  );
    GetChildPos( hWndRight, &xRight, &yRight, &cxRight, &cyRight );

    cxTot = cxLeft + cxRight + lpCnt->nBarWidth;
    
    // Resize window widths to reflect new split (Y org/height never changes).
    cxLeft  = xNewSplit - xLeft;
    xRight  = xNewSplit + lpCnt->nBarWidth;
    cxRight = cxTot - (xNewSplit - xLeft + lpCnt->nBarWidth);

    // Move the split children without repainting to avoid flashing.
    MoveWindow( hWndLeft,  xLeft,  yLeft,  cxLeft,  cyLeft,  FALSE );
    MoveWindow( hWndRight, xRight, yRight, cxRight, cyRight, FALSE );

    // Now repaint the entire frame.
    InvalidateRect( hWndFrame, NULL, TRUE );
    UpdateWindow( hWndFrame );

    // Notify the assoc wnd of the moved split bar.
    NotifyAssocWnd( hWndRight, lpCnt, CN_SPLITBAR_MOVED, 0, NULL, NULL, xMovSplit, 0, 0, NULL );

    return;
    }

/****************************************************************************
 * DeleteSplitBar
 *
 * Purpose:
 *  Deletes the split bar between 2 windows by destroying the window on the
 *  left side of the split bar. The origin of the right window becomes that
 *  of the left window. The width of the right window increases by the width
 *  of the left window plus the split bar between them.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure
 *  int           xSplit      - X coord of the split bar
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI DeleteSplitBar( LPCONTAINER lpCnt, int xSplit )
    {
    HWND  hWndLeft, hWndRight;
    LPSPLIT lpSplitBar;
    int   xChild, yChild, cxChild, cyChild;
    int   xLeft, cxLeft, xRight, cxRight;
    int   yLeft, cyLeft, yRight, cyRight;

    if( lpSplitBar = GetSplitBar( lpCnt, xSplit ) )
      {
      hWndLeft  = lpSplitBar->hWndLeft;
      hWndRight = lpSplitBar->hWndRight;
      }
    else
      {
      return;
      }

    // Get the origin and extents of the left and right wnds.
    GetChildPos( hWndLeft,  &xLeft,  &yLeft,  &cxLeft,  &cyLeft  );
    GetChildPos( hWndRight, &xRight, &yRight, &cxRight, &cyRight );

    // Make sure the remaining split child has the focus. We do it
    // before destroying the window so no focus msg will be sent.
    if( GetFocus() != lpCnt->hWnd1stChild )
      SetFocus( lpCnt->hWnd1stChild );

    // Get rid of the left window.
    DestroyWindow( hWndLeft );
   
    // New origin of right window will be that of the left window. The new
    // width of the right window will be the total of the 2 plus split bar.
    xChild  = xLeft;
    yChild  = yLeft;
    cxChild = cxLeft + cxRight + lpCnt->nBarWidth;
    cyChild = cyLeft;
    MoveWindow( hWndRight, xChild, yChild, cxChild, cyChild,  TRUE );
   
    // Clear these just to make sure no one is using them.
    lpSplitBar->hWndLeft  = NULL;
    lpSplitBar->hWndRight = NULL;
    lpSplitBar->xSplitStr = -1;
    lpSplitBar->xSplitEnd = -1;

    lpCnt->bIsSplit = FALSE;

    // Notify the assoc wnd of the deleted split bar.
    NotifyAssocWnd( hWndRight, lpCnt, CN_SPLITBAR_DELETED, 0, NULL, NULL, xSplit, 0, 0, NULL );

    return;
    }

/****************************************************************************
 * GetChildPos
 *
 * Purpose:
 *  Gets the origin and extents of a child window in FRAME CLIENT COORDS.
 *  Called when you need to get the positioning for a child of the frame.
 *
 * Parameters:
 *  HWND          hWnd        - handle to child window of the frame
 *  LPINT         lpXorg      - returned X origin of hWnd in frame client coords
 *  LPINT         lpYorg      - returned Y origin of hWnd in frame client coords
 *  LPINT         lpWidth     - returned width    of hWnd in frame client coords
 *  LPINT         lpHeight    - returned height   of hWnd in frame client coords
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI GetChildPos( HWND hWnd, LPINT lpXorg, LPINT lpYorg, LPINT lpWidth, LPINT lpHeight )
    {
    HWND  hWndFrame=GetParent(hWnd);
    RECT  rect;
    POINT ptLT, ptRB;
    int   xOrg, yOrg, cxExt, cyExt;

    if( !hWndFrame || !hWnd )
      return;

    // Get the screen origin and extents of the child.
    GetWindowRect( hWnd, &rect );
    ptLT.x = rect.left;
    ptLT.y = rect.top;
    ptRB.x = rect.right;
    ptRB.y = rect.bottom;

    // Convert into frame client coords.
    ScreenToClient( hWndFrame, &ptLT );
    ScreenToClient( hWndFrame, &ptRB );
    xOrg  = ptLT.x;
    yOrg  = ptLT.y;
    cxExt = ptRB.x - xOrg;
    cyExt = ptRB.y - yOrg;

    // Return the origin and extents.
    *lpXorg   = xOrg;
    *lpYorg   = yOrg;
    *lpWidth  = cxExt;
    *lpHeight = cyExt;
    }

/****************************************************************************
 * GetChildAtPt
 *
 * Purpose:
 *  Gets the child of the frame located at a specified point.
 *
 *  HWND          hWndFrame   - handle to the frame window
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xCoord      - Xcoord in frame client coords
 *
 * Return Value:
 *  HWND          handle to child window at xCoord
 ****************************************************************************/

HWND WINAPI GetChildAtPt( HWND hWndFrame, LPCONTAINER lpCnt, int xCoord )
    {
    // Just hard code this for now until multiple split bars are supported.
    // Until then this function should never be called if hWnd1stChild is
    // already split. We will eventually have to traverse the children of 
    // the frame to find out which child xCoord falls in. 

    return lpCnt->hWnd1stChild;
    }

/****************************************************************************
 * GetSplitBar
 *
 * Purpose:
 *  Gets the info for the split bar located at xCoord.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to existing CONTAINER structure.
 *  int           xCoord      - X coord potentially within a split bar
 *
 * Return Value:
 *  LPSPLIT       ptr to split bar if one was found at xCoord; else NULL;
 ****************************************************************************/

LPSPLIT WINAPI GetSplitBar( LPCONTAINER lpCnt, int xCoord )
    {
    LPSPLIT lpSplitBar=NULL;

    // Just hard code this for now. Will eventually have to keep a
    // linked list of split bar structs and search thru them.
    if( lpCnt->bIsSplit && xCoord >= lpCnt->SplitBar.xSplitStr &&
                    xCoord <= lpCnt->SplitBar.xSplitEnd )
      {
      lpSplitBar = &lpCnt->SplitBar;
      }

    return lpSplitBar;
    }
