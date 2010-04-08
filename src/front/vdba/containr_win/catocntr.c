/****************************************************************************
 * Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * CATOCNTR.C
 *
 * Contains the main window procedure for the Container control.
 ****************************************************************************/

/*
**  History:
**	21-Feb-2001 (ahaal01)
**	    For AIX (rs4_us5) use "getpid".
**	01-Nov-2001 (hanje04)
**	    Added Linux to platforms using getpid instead if _getpid
**  02-Jan-2002 (noifr01)
**      (SIR 99596) cleaned up some resources
*/

#include <windows.h>
#include <windowsx.h>  // Need this for the memory macros
#include <stdlib.h>
#include <malloc.h>
#include <process.h>
#include "cntdll.h"
#ifdef REALIZER
#include "custctrl.h"  // Realizer messaging
#endif

// Array of default colors, matching the order of CNTCOLOR_* values.
int crDefColor[CNTCOLORS]={ COLOR_WINDOWTEXT,    // title
                            COLOR_WINDOWTEXT,    // field titles
                            COLOR_WINDOWTEXT,    // record text
                            COLOR_WINDOWTEXT,    // grid
                            COLOR_WINDOW,        // cnt background
                            COLOR_HIGHLIGHT,     // selected text bkgrnd
                            COLOR_HIGHLIGHTTEXT, // selected text
                            COLOR_BTNFACE,       // cnt title background
                            COLOR_BTNFACE,       // field title background
                            COLOR_BTNFACE,       // field data background
                            COLOR_BTNHIGHLIGHT,  // highlight for 3D look
                            COLOR_BTNSHADOW,     // shadow for 3D look
                            COLOR_WINDOWTEXT,    // title button text
                            COLOR_BTNFACE,       // title button background
                            COLOR_WINDOWTEXT,    // field button text
                            COLOR_BTNFACE,       // field button background
                            COLOR_BTNFACE,       // unused field space color
                            COLOR_BTNSHADOW      // in-use hatch color
                           };

// Prototypes for local helper functions.
VOID NEAR CheckCntFocus( LPCONTAINER lpCnt );
VOID NEAR CopyFldList( HWND hCntWnd, LPCNTINFO lpCInew, LPCNTINFO lpCIold );
VOID NEAR CreateDefaultFont( HWND hWnd, LPCONTAINER lpCnt );
VOID NEAR GetResStr( int nResID, LPSTR lpBuf, UINT cBytes );
int  NEAR GetResStr2Int( int nResID, LPSTR lpBuf, UINT cBytes );

// Prototypes for Frame msg cracker functions.
VOID Frm_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Frm_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
VOID Frm_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest );
BOOL Frm_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Frm_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct );
BOOL Frm_OnWindowPosChanging( HWND hWnd, LPWINDOWPOS lpWindowPos );
VOID Frm_OnSize( HWND hWnd, UINT state, int cx, int cy );
VOID Frm_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Frm_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags );
VOID Frm_OnSetFocus( HWND hWnd, HWND hwndOldFocus );
BOOL Frm_OnEraseBkgnd( HWND hWnd, HDC hDC );
VOID Frm_OnPaint( HWND hWnd );

/****************************************************************************
 * FrameWndProc
 *
 * Purpose:
 *  Frame Window Procedure for the Container custom control. A frame wnd
 *  is necessary only for supporting split bars. A split bar is actually
 *  a space between 2 child container windows.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI FrameWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    switch( iMsg )
      {
      HANDLE_MSG( hWnd, WM_NCLBUTTONDOWN,  Frm_OnNCLButtonDown );
      HANDLE_MSG( hWnd, WM_NCMBUTTONDOWN,  Frm_OnNCMButtonDown );
      HANDLE_MSG( hWnd, WM_NCRBUTTONDOWN,  Frm_OnNCRButtonDown );
      HANDLE_MSG( hWnd, WM_NCCREATE,       Frm_OnNCCreate      );
      HANDLE_MSG( hWnd, WM_CREATE,         Frm_OnCreate        );
      HANDLE_MSG( hWnd, WM_WINDOWPOSCHANGING, Frm_OnWindowPosChanging );
      HANDLE_MSG( hWnd, WM_SIZE,           Frm_OnSize          );
      HANDLE_MSG( hWnd, WM_LBUTTONDOWN,    Frm_OnLButtonDown   );
      HANDLE_MSG( hWnd, WM_RBUTTONDBLCLK,  Frm_OnRButtonDown   );
      HANDLE_MSG( hWnd, WM_SETFOCUS,       Frm_OnSetFocus      );
      HANDLE_MSG( hWnd, WM_ERASEBKGND,     Frm_OnEraseBkgnd    );
      HANDLE_MSG( hWnd, WM_PAINT,          Frm_OnPaint         );

#ifdef REALIZER
     /*---------------------------------------------------------------------*
      * REALIZER Message processing
      *---------------------------------------------------------------------*/

      // This is a special message sent only from Realizer. Its purpose
      // is to allow us to 'steal' certain keystrokes that normally the
      // container (or any other control) would not receive.
      case WM_KEYDOWN + 0x1000:
        switch( wParam )
          {
          case VK_TAB:
          case VK_ESCAPE:
          case VK_RETURN:
          case VK_INSERT:
            {
            // Get a pointer to the container structure for this control.
            LPCONTAINER lpCnt = GETCNT(hWnd);

            // Tell Realizer that we are going to process this keystroke.
            // Realizer should have registered as the AUX window.
            if( lpCnt->lpCI->hAuxWnd )
              {
              SendMessage( lpCnt->lpCI->hAuxWnd, WM_USER+2, 0, 0L);

              // Now send the keystroke to the container child window.
              FORWARD_WM_KEYDOWN( GetFocus(), wParam, 0, 0, SendMessage );
              }
            }
            break;
          }
        return 0L;

      case CCM_SETNUM:
        {
        LPLONG lplStyles=(LPLONG)lParam;
        LONG   lStyle=0L;
        int    i;

        // For SETNUM with the container we only care about the styles.
        if( *lplStyles == CCSTYLE )   
          {
          // For Realizer style types, the first wParam is a count of 
          // style modifiers, and lParam points to an array of longs 
          // containing the individual style flags.

          // Eat the CCSTYLE type and get to the real styles.
          lplStyles++;

          // Collect all the styles being passed in.
          for( i = 1; i < (int) wParam; i++, lplStyles++ )
            lStyle |= *lplStyles;

          // Apply these styles to the container.
          CntStyleSet( hWnd, (DWORD)lStyle );
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
        return CNT_DEFWNDPROC( hWnd, iMsg, wParam, lParam );
      }

    return 0L;
    }

/****************************************************************************
 * ContainerWndProc
 *
 * Purpose:
 *  Main Window Procedure for the Container custom control.  Handles all
 *  window messages and routes them to the appropriate view handler.
 *
 * Parameters:
 *  Standard for WndProc
 *
 * Return Value:
 *  LRESULT       Varies with the message.
 ****************************************************************************/

LRESULT WINAPI ContainerWndProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    UINT        iView;

   /*-----------------------------------------------------------------------*
    * The pointer to the window's CONTAINER structure is only NULL before
    * processing the WM_NCCREATE msg (where the memory is allocated). If this
    * is the case we will let DetailViewWndProc handle the NCCREATE msg.
    * This msg is handled identically in all views.
    *-----------------------------------------------------------------------*/
    if( lpCnt )
      iView = lpCnt->iView;
    else
      iView = CV_DETAIL;

    // Route the msg to the active view's handler.
    switch( iView )
    {
      case  CV_TEXT:
      case (CV_TEXT | CV_FLOW):
        return TextViewWndProc( hWnd, iMsg, wParam, lParam );
      case  CV_NAME:
      case (CV_NAME | CV_FLOW):
      case (CV_NAME | CV_MINI):
      case (CV_NAME | CV_MINI | CV_FLOW):
        return NameViewWndProc( hWnd, iMsg, wParam, lParam );
      case  CV_ICON:
      case (CV_ICON | CV_MINI):
        return IconViewWndProc( hWnd, iMsg, wParam, lParam );
      default:
        return DetailViewWndProc( hWnd, iMsg, wParam, lParam );
    }

    return 0L;
    }

/****************************************************************************
 * FrameWndProc Message Cracker Callback Functions
 *
 * Purpose:
 *  Functions to process each message received by the Container WndProc.
 *  This is a Win16/Win32 portable method for message handling.
 *
 * Return Value:
 *  Varies according to the message being processed.
 ****************************************************************************/

VOID Frm_OnNCLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // If a non-client mouse hit occurs send the focus to the
    // 1st child if the container does not have the focus.
    CheckCntFocus( lpCnt );

    // Forward the NC msg to the def proc.
    FORWARD_WM_NCLBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
    }

VOID Frm_OnNCMButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // If a non-client mouse hit occurs send the focus to the
    // 1st child if the container does not have the focus.
    CheckCntFocus( lpCnt );

    // Forward the NC msg to the def proc.
    FORWARD_WM_NCMBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
    }

VOID Frm_OnNCRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // If a non-client mouse hit occurs send the focus to the
    // 1st child if the container does not have the focus.
    CheckCntFocus( lpCnt );

    // Forward the NC msg to the def proc.
    FORWARD_WM_NCRBUTTONDOWN( hWnd, fDoubleClick, x, y, codeHitTest, CNT_DEFWNDPROC );
    return;
    }

BOOL Frm_OnNCCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPCONTAINER lpCntNew;
    LPCNTINFO   lpCntInfo;
    DWORD       dwStyle;

    // Alloc memory for the control's CONTAINER struct
    // and store the handle in the window's extra bytes.
/*    lpCntNew = (LPCONTAINER) GlobalAllocPtr( GHND, LEN_CONTAINER );*/
    lpCntNew = (LPCONTAINER) calloc( 1, LEN_CONTAINER );
    if( lpCntNew == NULL )
      return 0L;

    // Save the container pointer in the window extra bytes.
    SetWindowLong( hWnd, GWL_CONTAINERHMEM, (LONG)lpCntNew );

    // User should never use the window scrollbar styles.
    dwStyle  = (DWORD) GetWindowLong( hWnd, GWL_STYLE );
    dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);
    lpCreateStruct->style &= ~(WS_VSCROLL | WS_HSCROLL);
    SetWindowLong( hWnd, GWL_STYLE, (LONG)dwStyle );

    // Alloc memory for the container info struct using macros from 
    // windowsx.h. Note that there is no messing with handle locking.
/*    lpCntInfo = (LPCNTINFO) GlobalAllocPtr( GHND, LEN_CNTINFO );*/
    lpCntInfo = (LPCNTINFO) calloc( 1, LEN_CNTINFO );
    if( lpCntInfo == NULL )
      return 0L;

    // Save the container info pointer in the container struct.
    lpCntNew->lpCI = lpCntInfo;

    // Clear out all initial states.
    StateClear( lpCntNew, CNTSTATE_ALL );

    return FORWARD_WM_NCCREATE( hWnd, lpCreateStruct, CNT_DEFWNDPROC );
    }

BOOL Frm_OnCreate( HWND hWnd, LPCREATESTRUCT lpCreateStruct )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // Save the frame window handle in the struct.
    lpCnt->hWndFrame = hWnd;

    // Our associate window is the parent by default.
    lpCnt->hWndAssociate = GetParent( hWnd );

    // if no view specified as of yet, set default view to detailed view
    if (!lpCnt->iView)
      lpCnt->iView = CV_DETAIL;
    
    // Save the container styles in the struct.
    lpCnt->dwStyle = lpCreateStruct->style;

    // set starting index value for drag operations
    lpCnt->nDrgop = 0;

    // set starting index value for received drop operations
    lpCnt->nDropop = 0;

    // get our process ID for later
#if defined(aix4) || defined(__linux__) /* rs4_us5 */
    lpCnt->dwPid = getpid();
#else
    lpCnt->dwPid = _getpid();
#endif /* rs4_us5 */

    // set message id to -1
    lpCnt->iInputID = -1;
    
    // load our custom messages 
    lpCnt->uWmDragOver = RegisterWindowMessage(WM_CNTDRAGOVER);
    if(lpCnt->uWmDragOver == 0)
      return(FALSE);  

    lpCnt->uWmDrop = RegisterWindowMessage(WM_CNTDROP);
    if(lpCnt->uWmDrop  == 0)
      return(FALSE);

    lpCnt->uWmDragLeave =RegisterWindowMessage(WM_CNTDRAGLEAVE);
    if(lpCnt->uWmDragLeave == 0)
      return(FALSE);

    lpCnt->uWmDragComplete =RegisterWindowMessage(WM_CNTDROPCOMPLETE);
    if(lpCnt->uWmDragComplete == 0)
      return(FALSE);

    lpCnt->uWmRenderData =RegisterWindowMessage(WM_CNTRENDERDATA);
    if(lpCnt->uWmRenderData == 0)
      return(FALSE);

    lpCnt->uWmRenderComplete =RegisterWindowMessage(WM_CNTRENDERCOMPLETE);
    if(lpCnt->uWmRenderComplete == 0)
      return(FALSE);

    // init return message value;
    lpCnt->lReturnMsgVal = 0L;
  
    // If the app supplied a caption title, set it.
    if( lpCreateStruct->lpszName && lstrlen( lpCreateStruct->lpszName ) )
      SetWindowText( hWnd, lpCreateStruct->lpszName );

    // Set the arrow as the default cursor.
    lpCnt->lpCI->hCursor = LoadCursor( NULL, IDC_ARROW );

    // Set the title button and field title button cursors.
    lpCnt->lpCI->hCurTtlBtn    = LoadCursor( NULL, IDC_ARROW );
    lpCnt->lpCI->hCurFldTtlBtn = LoadCursor( NULL, IDC_ARROW );

    // Load the sizing, moving, and splitting cursors.
    lpCnt->lpCI->hCurFldSizing = LoadCursor( NULL,IDC_SIZEWE );
    lpCnt->lpCI->hCurFldMoving = LoadCursor( NULL,IDC_CROSS);
    lpCnt->hCurSplitBar        = LoadCursor( NULL,IDC_ARROW);


    // Create the default font and set it as the general font.
    CreateDefaultFont( hWnd, lpCnt );
    lpCnt->lpCI->hFont = lpCnt->hFontDefault;

    // Set default vertical scrolling range.
    lpCnt->iMin = IDEFAULTMIN;
    lpCnt->iMax = IDEFAULTMAX;
    lpCnt->iPos = IDEFAULTPOS;

    // init upper left of icon viewspace
    lpCnt->lpCI->rcViewSpace.top  = 0;
    lpCnt->lpCI->rcViewSpace.left = 0;
    
    // Set split bar metrics.
    lpCnt->nBarWidth = GetSystemMetrics( SM_CYDLGFRAME );
    lpCnt->nHalfBarWidth = lpCnt->nBarWidth / 2;
    
    // Set minimum width of the right window after a split.
    lpCnt->nMinWndWidth = GetSystemMetrics( SM_CXVSCROLL );

    // Init flag indicating whether or not to do scroll-to-focus behavior.
    lpCnt->bScrollFocusRect = TRUE;

    // Clear out all initial states.
    StateClear(lpCnt, CNTSTATE_ALL);

    // Initialize the container colors.
    CntColorManager( hWnd, lpCnt, CLR_INIT );

   /*-------------------------------------------------------------------*
    * Create the permanent 1st split child.
    *-------------------------------------------------------------------*/

    // These styles should never be used for the children.
    lpCnt->dwStyle &= ~(WS_CAPTION | WS_VSCROLL | WS_HSCROLL | WS_SYSMENU |
                        WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX );

    // Always want these on for the children.
    lpCnt->dwStyle |= WS_BORDER | WS_CHILD | WS_CLIPCHILDREN;

    // Pass the existing container struct so the frame and 1st child will
    // share the same container struct. All other children get their own.

    lpCnt->hWnd1stChild = CreateWindow( CNTCHILD_CLASS, "", lpCnt->dwStyle,
                          0, 0, 0, 0,
                          hWnd, NULL, hInstCnt, (LPVOID)lpCnt );

    // Only the 1st child window will have a vertical scroll bar.
    // The left window without a vert scroll bar is created after a split.
    SetWindowLong( lpCnt->hWnd1stChild, GWL_SHOWVSCROLL, TRUE );

    // NOTE: When using a msg cracker for WM_CREATE we must
    //       return TRUE if all is well instead of 0.
    return TRUE;
    }

BOOL Frm_OnWindowPosChanging( HWND hWnd, LPWINDOWPOS lpWindowPos )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    int  cx, cy;
    int  cxUnused, cyPartial;
    BOOL bHscroll, bVscroll;

    if( IsIconic( hWnd ) )
      return 0;

    if( !lpWindowPos->cx || !lpWindowPos->cy || lpCnt->bIsSplit )
      return 0;

    // Adjust the width or height if the styles are enabled.
    if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
         (!(lpCnt->iView & CV_ICON)) )
      {
      cx = lpWindowPos->cx;
      cy = lpWindowPos->cy;

      CntSizeCheck( hWnd, cx, cy, 0, &cxUnused, &cyPartial, &bHscroll, &bVscroll );

      if( lpCnt->dwStyle & CTS_INTEGRALWIDTH )
        lpWindowPos->cx -= cxUnused;

      if( lpCnt->dwStyle & CTS_INTEGRALHEIGHT )
        lpWindowPos->cy -= cyPartial;
      }

    return 0;
    }

VOID Frm_OnSize( HWND hWnd, UINT state, int cx, int cy )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    RECT rect;
    int nHeight, nWidth;
    int xChild, cxChild;
    int yChild, cyChild;
    int xLeft,  cxLeft;
    int xRight, cxRight;
    int xNewSplit, xOldSplit;

    // If split, resize children proportional to previous sizes.
    // If not split, the child takes up the entire client area.
    if( !IsIconic( hWnd ) )
      {
      // Update the original container size if not sizing myself.
      // We want to keep the original width and height so that as
      // the container goes thru various size adjustments for the
      // integral width and height it is always based on the original
      // size the user specified. We do this to prevent us from continually
      // shrinking as scroll bars come and go.
      if( !lpCnt->bIntegralSizeChg )
        {
        GetWindowRect( hWnd, &rect );
        lpCnt->cxWndOrig = rect.right - rect.left;
        if( !lpCnt->bRetainBaseHt )
          lpCnt->cyWndOrig = rect.bottom - rect.top;
        }

      nWidth  = cx;
      nHeight = cy;

      // Make child window bigger all around by 1 to hide its border.
      xChild  = yChild = -1;
      cxChild = nWidth  + 2;
      cyChild = nHeight + 2;

      // TEST VARIED CHILD SIZES
      /*yChild+=10; cyChild-=20;*/
      /*xChild+=10; cxChild-=20;*/

      if( !lpCnt->bIsSplit )
        {
        MoveWindow( lpCnt->hWnd1stChild, xChild, yChild, cxChild, cyChild, TRUE );
        }
      else
        {
        xOldSplit = lpCnt->SplitBar.xSplitStr;  
        xNewSplit = (int)(lpCnt->SplitBar.fSplitPerCent * nWidth);

        // Account for a vertical scroll bar if it is showing.
        if( lpCnt->dwStyle & CTS_VERTSCROLL && 
            lpCnt->lpCI->dwRecordNbr > (DWORD)lpCnt->lpCI->nRecsDisplayed-1 )
          xNewSplit = min(xNewSplit, nWidth - lpCnt->nMinWndWidth - lpCnt->nBarWidth + 1);
        else
          xNewSplit = min(xNewSplit, nWidth - lpCnt->nBarWidth + 1);

        lpCnt->SplitBar.xSplitStr = xNewSplit;  
        lpCnt->SplitBar.xSplitEnd = xNewSplit + lpCnt->nBarWidth - 1;

        xLeft   = -1;
        cxLeft  = xNewSplit - xLeft;
        xRight  = xNewSplit + lpCnt->nBarWidth;
        cxRight = nWidth+2 - (cxLeft + lpCnt->nBarWidth);

        // Move the split children without repainting to avoid flashing.
        MoveWindow( lpCnt->SplitBar.hWndLeft,  xLeft,  yChild, cxLeft,  cyChild, FALSE );
        MoveWindow( lpCnt->SplitBar.hWndRight, xRight, yChild, cxRight, cyChild, FALSE );

        // Now repaint the entire frame.
        InvalidateRect( hWnd, NULL, TRUE );
        UpdateWindow( hWnd );
        }
      }

    return;
    }

VOID Frm_OnLButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT       pt;

    // If a mouse hit occurs send the focus to the 1st child
    // if the container does not currently have the focus.
    CheckCntFocus( lpCnt );

    if( !fDoubleClick )
      {
      // User wants to move a split bar located at this X coord.
      pt.x = x;
      pt.y = y;
      if( !IsIconic( hWnd ) && lpCnt->bIsSplit )
        SplitBarManager( hWnd, lpCnt, SPLITBAR_MOVE, &pt );
      }

    return;
    }

VOID Frm_OnRButtonDown( HWND hWnd, BOOL fDoubleClick, int x, int y, UINT keyFlags )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT       pt;

    // If a mouse hit occurs send the focus to the 1st child
    // if the container does not currently have the focus.
    CheckCntFocus( lpCnt );

    if( fDoubleClick )
      {
      // User wants to delete split bar located at this X coord.
      pt.x = x;
      pt.y = y;
      if( !IsIconic( hWnd ) && lpCnt->bIsSplit && !(lpCnt->lpCI->flCntAttr & CA_APPSPLITABLE) )
        SplitBarManager( hWnd, lpCnt, SPLITBAR_DELETE, &pt );
      }

    return;
    }

VOID Frm_OnSetFocus( HWND hWnd, HWND hwndOldFocus )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    // The frame window never retains the focus.
    // Set the focus back on the split child which had it last.
    if( IsWindow( lpCnt->hWndLastFocus ) )
      SetFocus( lpCnt->hWndLastFocus );
    else if( IsWindow( lpCnt->hWnd1stChild ) )
      SetFocus( lpCnt->hWnd1stChild );

    return;
    }

BOOL Frm_OnEraseBkgnd( HWND hWnd, HDC hDC )
    {
    // Intercept Windows attempts to erase the frame backgound.
    // This is done by not letting this message get to the DefWndProc.

    // Uncommenting this line will let Windows repaint the background.
//    return FORWARD_WM_ERASEBKGND( hWnd, hDC, CNT_DEFWNDPROC );
    return TRUE;
    }

VOID Frm_OnPaint( HWND hWnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    PAINTSTRUCT ps;
    HDC         hDC;
    RECT        rect;

    hDC = BeginPaint( hWnd, &ps );
    
    // The only painting we need to do for the frame is the gap between
    // the 2 split windows (the 'splitbar'). Originally this was not 
    // necessary because I let Windows erase the background. This resulted
    // in the splitbar area always being painted in the window background
    // color. However, this also resulted in the entire container being
    // repainted unnecessarily. It was unnecessary because the child windows
    // cover the frame completely except for the splitbar gap when the 
    // container is split. Therefore we execute the following code to take
    // care of the splitbar gap painting.
    if( lpCnt->bIsSplit )
      {
      CopyRect( &rect, &ps.rcPaint );
      rect.left  = lpCnt->SplitBar.xSplitStr;
      rect.right = lpCnt->SplitBar.xSplitEnd + 1;

      // Paint the splitbar gap in the window background color.
      SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
      ExtTextOut( hDC, rect.left, rect.top, ETO_OPAQUE, &rect, NULL, 0, NULL );
      }

    EndPaint( hWnd, &ps );

    return;
    }

/****************************************************************************
 * CheckCntFocus
 *
 * Purpose:
 *  If the frame window gets a mouse hit and no container child
 *  currently has the focus the focus is set on the 1st child.
 *
 * Parameters:
 *  LPCONTAINER   lpCnt       - pointer to control data structure.
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR CheckCntFocus( LPCONTAINER lpCnt )
    {
    LPCONTAINER lpCntFrame;
    BOOL        bSetFocus=FALSE;

    // If the container does not have the focus, set it on the 1st child.
    if( lpCnt->bIsSplit )
      {
      lpCntFrame = GETCNT( lpCnt->hWndFrame );
      if( GetFocus() != lpCnt->hWnd1stChild &&
          GetFocus() != lpCntFrame->SplitBar.hWndLeft )
        bSetFocus = TRUE;
      }
    else
      {
      if( GetFocus() != lpCnt->hWnd1stChild )
        bSetFocus = TRUE;
      }

    if( bSetFocus )
      {
      SetFocus( lpCnt->hWnd1stChild );
      if( !StateTest( lpCnt, CNTSTATE_HASFOCUS ) )
        {
        // The container should have gotten a WM_SETFOCUS msg which sets
        // the HASFOCUS state but in some cases it is not sent. If the
        // container didn't get the WM_SETFOCUS msg we will send it now
        // to ensure that the focus rect is painted correctly.
        FORWARD_WM_SETFOCUS( lpCnt->hWnd1stChild, NULL, SendMessage );
        }
      }

    return;
    }

/****************************************************************************
 * CopyFldList
 *
 * Purpose:
 *  Copies the field list from one CNTINFO struct to another. This function
 *  assumes that allocation for the new field nodes is not done yet and
 *  will be done here.
 *
 *  NOTE:  This function is currently not used.
 *
 * Parameters:
 *  HWND          hCntWnd     - Container window handle 
 *  LPCNTINFO     lpCInew     - pointer to destination struct
 *  LPCNTINFO     lpCIold     - pointer to source struct
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR CopyFldList( HWND hCntWnd, LPCNTINFO lpCInew, LPCNTINFO lpCIold )
    {
    LPFIELDINFO lpCol, lpNewCol;

    // Get the pointer to the 1st field.
    lpCol = lpCIold->lpFieldHead;
    if( !lpCol )
      return;

    while( lpCol )
      {
      // Alloc a new field node.
      lpNewCol = CntNewFldInfo();

      // Copy the old field node contents into the new one.
      *lpNewCol = *lpCol;

      // Null these pointers out so new storage will be allocated.
      lpNewCol->lpFTitle     = NULL;
      lpNewCol->lpUserData   = NULL;
      lpNewCol->lpDescriptor = NULL;

      // Null out the Next and Prev ptrs just to make sure.
      lpNewCol->lpNext       = NULL;        
      lpNewCol->lpPrev       = NULL;        
      
      // Copy the field title from the old into the new.
      if( lpCol->wTitleLen && lpCol->lpFTitle )
        CntFldTtlSet( hCntWnd, lpNewCol, lpCol->lpFTitle, lpCol->wTitleLen );
    
      // Copy the field user data from the old into the new.
      if( lpCol->wUserBytes && lpCol->lpUserData )
        CntFldUserSet( lpNewCol, lpCol->lpUserData, lpCol->wUserBytes );
    
      // Copy the field descriptor from the old into the new.
      if( lpCol->wDescBytes && lpCol->lpDescriptor )
        CntFldDescSet( lpNewCol, lpCol->lpDescriptor, lpCol->wDescBytes );
    
      // Now add this field to the new container field list.
      CntAddFldTail( hCntWnd, lpNewCol );

      // Advance to next field node.
      lpCol = lpCol->lpNext;
      }
    }

/****************************************************************************
 * NotifyAssocWnd
 *
 * Purpose:
 *  Sends a notification of an event to the associate window.
 *  In most cases this would be the parent window. If there is a char,
 *  record, or field associated with the current notification it is
 *  passed in and temporarily held until the notification is processed.
 *
 * Parameter:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to control data structure.
 *  UINT          wEvent      - identifier of the event.
 *  UINT          wOemCharVal - OEM value of char pressed (only for CN_CHARHIT)
 *  LPRECORDCORE  lpRec       - pointer to record of interest 
 *  LPFIELDINFO   lpFld       - pointer to field of interest 
 *  LONG          lInc        - increment for a scrolling notification or split bar Xcoord
 *  BOOL          bShiftKey   - state of the shift key   (TRUE means pressed)
 *  BOOL          bCtrlKey    - state of the control key (TRUE means pressed)
 *  LPVOID        lpUserData  - pointer to user data (only used via CntNotifyAssoc)
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI NotifyAssocWnd( HWND hWnd, LPCONTAINER lpCnt, UINT wEvent, UINT wOemCharVal,
                            LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                            BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData )
    {
    HGLOBAL   hEvent;
    LPCNEVENT lpCNEvent;
    BYTE      cbOemVal;
    BYTE      cbAnsiVal;
    UINT      wAnsiCharVal=0;

    // If a char is being passed in translate it to ansi.
    if( wOemCharVal )
      {
      cbOemVal = (BYTE) wOemCharVal;
      OemToAnsiBuff( &cbOemVal, &cbAnsiVal, 1 );
      wAnsiCharVal = cbAnsiVal;
      }

    if( lpCnt->dwStyle & CTS_ASYNCNOTIFY )
      {
      hEvent = GlobalAlloc( GHND, LEN_CNEVENT );
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
        {
        lpCNEvent->hWndCurCNSender = hWnd;
        lpCNEvent->wCurNotify      = wEvent;
        lpCNEvent->wCurAnsiChar    = wAnsiCharVal;
        lpCNEvent->lpCurCNRec      = lpRec;
        lpCNEvent->lpCurCNFld      = lpFld;
        lpCNEvent->lCurCNInc       = lInc;
        lpCNEvent->bCurCNShiftKey  = bShiftKey;
        lpCNEvent->bCurCNCtrlKey   = bCtrlKey;
        lpCNEvent->lpCurCNUser     = lpUserData;
        GlobalUnlock( hEvent );
        }

#ifdef WIN16
      FORWARD_WM_COMMAND( lpCnt->hWndAssociate, GetWindowID( lpCnt->hWndFrame ),
                          lpCnt->hWndFrame, hEvent, SendMessage );
#else
      // For Win32 we send a custom msg so that Realizer will not stop the msg.
      // If we send a wm_command msg Realizer looks at the hWnd that is supposed
      // to be in the lParam to determine where the msg goes. Since we are passing
      // a memory handle instead of a window handle in the lParam Realizer doesn't
      // know what to do with it and does not forward it to the form proc.
      FORWARD_CCM_CUSTOM( lpCnt->hWndAssociate, GetWindowID( lpCnt->hWndFrame ),
                          hEvent, wEvent, SendMessage );
#endif
      }
    else
      {
      // Set these in case someone needs to query the values during the
      // notification processing. 
      lpCnt->lpCI->hWndCurCNSender = hWnd;
      lpCnt->lpCI->wCurNotify      = wEvent;
      lpCnt->lpCI->wCurAnsiChar    = wAnsiCharVal;
      lpCnt->lpCI->lpCurCNRec      = lpRec;
      lpCnt->lpCI->lpCurCNFld      = lpFld;
      lpCnt->lCurCNInc             = lInc;
      lpCnt->lpCI->bCurCNShiftKey  = bShiftKey;
      lpCnt->lpCI->bCurCNCtrlKey   = bCtrlKey;
      lpCnt->lpCI->lpCurCNUser     = lpUserData;

      FORWARD_WM_COMMAND( lpCnt->hWndAssociate, GetWindowID( lpCnt->hWndFrame ),
                          lpCnt->hWndFrame, wEvent, SendMessage );

      // Clear the notification message and values.
      lpCnt->lpCI->hWndCurCNSender = NULL;
      lpCnt->lpCI->wCurNotify      = 0;
      lpCnt->lpCI->wCurAnsiChar    = 0;
      lpCnt->lpCI->lpCurCNRec      = NULL;
      lpCnt->lpCI->lpCurCNFld      = NULL;
      lpCnt->lCurCNInc             = 0;
      lpCnt->lpCI->bCurCNShiftKey  = 0;
      lpCnt->lpCI->bCurCNCtrlKey   = 0;
      lpCnt->lpCI->lpCurCNUser     = NULL;
      }
    }

/****************************************************************************
 * NotifyAssocWndEx
 *
 * Purpose:
 *  Sends a notification of an event to the associate window.
 *  In most cases this would be the parent window. If there is a char,
 *  record, or field associated with the current notification it is
 *  passed in and temporarily held until the notification is processed.
 *
 * NOTE The only difference in this function and NotifyAssocWndEx is that
 *      this uses a direct call to send message and has a return value
 *      from SendMessage
 *
 * Parameter:
 *  HWND          hWnd        - handle to the control window.
 *  LPCONTAINER   lpCnt       - pointer to control data structure.
 *  UINT          wEvent      - identifier of the event.
 *  UINT          wOemCharVal - OEM value of char pressed (only for CN_CHARHIT)
 *  LPRECORDCORE  lpRec       - pointer to record of interest 
 *  LPFIELDINFO   lpFld       - pointer to field of interest 
 *  LONG          lInc        - increment for a scrolling notification or split bar Xcoord
 *  BOOL          bShiftKey   - state of the shift key   (TRUE means pressed)
 *  BOOL          bCtrlKey    - state of the control key (TRUE means pressed)
 *  LPVOID        lpUserData  - pointer to user data (only used via CntNotifyAssoc)
 *
 * Return Value:
 *  lResult - based on owner return code
 ****************************************************************************/

LRESULT WINAPI NotifyAssocWndEx( HWND hWnd, LPCONTAINER lpCnt, UINT wEvent, UINT wOemCharVal,
                            LPRECORDCORE lpRec, LPFIELDINFO lpFld, LONG lInc, 
                            BOOL bShiftKey, BOOL bCtrlKey, LPVOID lpUserData )
{
    HGLOBAL   hEvent;
    LPCNEVENT lpCNEvent;
    BYTE      cbOemVal;
    BYTE      cbAnsiVal;
    UINT      wAnsiCharVal=0;
    LRESULT   lResult = 0;
    WPARAM    wParam;
    LPARAM    lParam;

    // If a char is being passed in translate it to ansi.
    if( wOemCharVal )
    {
      cbOemVal = (BYTE) wOemCharVal;
      OemToAnsiBuff( &cbOemVal, &cbAnsiVal, 1 );
      wAnsiCharVal = cbAnsiVal;
    }

    if( lpCnt->dwStyle & CTS_ASYNCNOTIFY )
    {
      hEvent = GlobalAlloc( GHND, LEN_CNEVENT );
      lpCNEvent = (LPCNEVENT) GlobalLock( hEvent );
      if( lpCNEvent )
      {
        lpCNEvent->hWndCurCNSender = hWnd;
        lpCNEvent->wCurNotify      = wEvent;
        lpCNEvent->wCurAnsiChar    = wAnsiCharVal;
        lpCNEvent->lpCurCNRec      = lpRec;
        lpCNEvent->lpCurCNFld      = lpFld;
        lpCNEvent->lCurCNInc       = lInc;
        lpCNEvent->bCurCNShiftKey  = bShiftKey;
        lpCNEvent->bCurCNCtrlKey   = bCtrlKey;
        lpCNEvent->lpCurCNUser     = lpUserData;
        GlobalUnlock( hEvent );
      }


#ifdef WIN16
      wParam = (WPARAM) (int) GetWindowID(lpCnt->hWndFrame);
      lParam = MAKELPARAM((UINT)lpCnt->hWndFrame,wEvent);

      lResult = SendMessage(lpCnt->hWndAssociate,WM_COMMAND,wParam,lParam);
#else
      // For Win32 we send a custom msg so that Realizer will not stop the msg.
      // If we send a wm_command msg Realizer looks at the hWnd that is supposed
      // to be in the lParam to determine where the msg goes. Since we are passing
      // a memory handle instead of a window handle in the lParam Realizer doesn't
      // know what to do with it and does not forward it to the form proc.
      FORWARD_CCM_CUSTOM( lpCnt->hWndAssociate, GetWindowID( lpCnt->hWndFrame ),
                          hEvent, wEvent, SendMessage );
#endif
    }
    else
    {
      // Set these in case someone needs to query the values during the
      // notification processing. 
      lpCnt->lpCI->hWndCurCNSender = hWnd;
      lpCnt->lpCI->wCurNotify      = wEvent;
      lpCnt->lpCI->wCurAnsiChar    = wAnsiCharVal;
      lpCnt->lpCI->lpCurCNRec      = lpRec;
      lpCnt->lpCI->lpCurCNFld      = lpFld;
      lpCnt->lCurCNInc             = lInc;
      lpCnt->lpCI->bCurCNShiftKey  = bShiftKey;
      lpCnt->lpCI->bCurCNCtrlKey   = bCtrlKey;
      lpCnt->lpCI->lpCurCNUser     = lpUserData;

#ifdef WIN32
      wParam = MAKEWPARAM(GetWindowID(lpCnt->hWndFrame),wEvent);
      lParam = (LPARAM)lpCnt->hWndFrame;
#else
      wParam = (WPARAM) (int) GetWindowID(lpCnt->hWndFrame);
      lParam = MAKELPARAM((UINT)lpCnt->hWndFrame,wEvent);
#endif

      SendMessage(lpCnt->hWndAssociate, WM_COMMAND, wParam, lParam);

      // get the message that the user set in their processing of the notification message
      lResult = lpCnt->lReturnMsgVal;
      lpCnt->lReturnMsgVal = 0L;
      
      // Clear the notification message and values.
      lpCnt->lpCI->hWndCurCNSender = NULL;
      lpCnt->lpCI->wCurNotify      = 0;
      lpCnt->lpCI->wCurAnsiChar    = 0;
      lpCnt->lpCI->lpCurCNRec      = NULL;
      lpCnt->lpCI->lpCurCNFld      = NULL;
      lpCnt->lCurCNInc             = 0;
      lpCnt->lpCI->bCurCNShiftKey  = 0;
      lpCnt->lpCI->bCurCNCtrlKey   = 0;
      lpCnt->lpCI->lpCurCNUser     = NULL;
    }
    
    return(lResult);
}

/****************************************************************************
 * InitTextMetrics
 *
 * Purpose:
 *  Calculates the character metrics for all fonts and scrolling values.
 *  Should be called at create time and anytime the following items chg:
 *       - any font
 *       - nbr of fields
 *       - nbr of title lines
 *       - nbr of field title lines
 *
 *  The above items affect the way the scrollbars are handled as well 
 *  as other things.
 *
 * Parameter:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI InitTextMetrics( HWND hWnd, LPCONTAINER lpCnt )
{
    HDC         hDC;
    TEXTMETRIC  tm;
    LPFIELDINFO lpCol;
    SIZE        Size;
    RECT        rect;
    int         cxCharTtl,     cyCharTtl;
    int         cxCharTtl2,    cyCharTtl2,    cyCharTtl4;
    int         cxCharFldTtl,  cyCharFldTtl;
    int         cxCharFldTtl2, cyCharFldTtl2, cyCharFldTtl4;
    int         cxChar,  cyChar,  cyRow;
    int         cxChar2, cyChar2, cyRow2;
    int         cxChar4, cyChar4, cyRow4;
    int         nMaxWidth=0;
    int         nCols=0;
    int         cxClient, cyClient;
    int         cxVscrollBar;
    int         cyTitleArea=0;
    int         cyColTitleArea=0;
    int         cyRecordArea;
    int         cyLineSpace;
    int         cxWidth, cyHeight;
    int         cxOldWidth, cyOldHeight;

    hDC = GetDC( hWnd );

    // If integral styles are on we need to watch for a size change.
    if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
          (!(lpCnt->iView & CV_ICON)) )
      {
      GetWindowRect( lpCnt->hWndFrame, &rect );
      cxOldWidth  = rect.right  - rect.left;
      cyOldHeight = rect.bottom - rect.top;
      }

   /*-----------------------------------------------------------------------*
    * Setup metrics for the container title font.
    *-----------------------------------------------------------------------*/

    // Select the title font if one has been defined.
    // If there is no title font defined, the general font is used.
    // If there is no general font defined, the default ansi font is used.
    if( lpCnt->lpCI->hFontTitle )
      SelectObject( hDC, lpCnt->lpCI->hFontTitle );
    else if( lpCnt->lpCI->hFont )
      SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // Get the font char width and height.
    GetTextMetrics( hDC, &tm );
    GetTextExtentPoint( hDC, "0", 1, &Size );
    cxCharTtl  = Size.cx - tm.tmOverhang;
/*    cxCharTtl  = LOWORD(GetTextExtent( hDC, "0", 1 ));*/
    cyCharTtl  = tm.tmHeight + tm.tmExternalLeading;

    // Set half and quarter char width and height dimensions.
    cxCharTtl2 = (cxCharTtl+1)  >> 1;
    cyCharTtl2 = (cyCharTtl+1)  >> 1;
    cyCharTtl4 = (cyCharTtl2+1) >> 1;

    // Save these dimensions in the container info struct.
    lpCnt->lpCI->cxCharTtl  = cxCharTtl;
    lpCnt->lpCI->cxCharTtl2 = cxCharTtl2;
    lpCnt->lpCI->cyCharTtl  = cyCharTtl;
    lpCnt->lpCI->cyCharTtl2 = cyCharTtl2;
    lpCnt->lpCI->cyCharTtl4 = cyCharTtl4;

   /*-----------------------------------------------------------------------*
    * Setup metrics for the container field titles font.
    *-----------------------------------------------------------------------*/

    // Select the field title font if one has been defined.
    // If there is no field title font defined, the general font is used.
    // If there is no general font defined, the default ansi font is used.
    if( lpCnt->lpCI->hFontColTitle )
      SelectObject( hDC, lpCnt->lpCI->hFontColTitle );
    else if( lpCnt->lpCI->hFont )
      SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // Get the font char width and height.
    GetTextMetrics( hDC, &tm );
    GetTextExtentPoint( hDC, "0", 1, &Size );
    cxCharFldTtl  = Size.cx - tm.tmOverhang;
/*    cxCharFldTtl  = LOWORD(GetTextExtent( hDC, "0", 1 ));*/
    cyCharFldTtl  = tm.tmHeight + tm.tmExternalLeading;

    // Set half char width and height dimensions.
    cxCharFldTtl2 = (cxCharFldTtl+1)  >> 1;
    cyCharFldTtl2 = (cyCharFldTtl+1)  >> 1;
    cyCharFldTtl4 = (cyCharFldTtl2+1) >> 1;

    // Save these dimensions in the container info struct.
    lpCnt->lpCI->cxCharFldTtl  = cxCharFldTtl;
    lpCnt->lpCI->cxCharFldTtl2 = cxCharFldTtl2;
    lpCnt->lpCI->cyCharFldTtl  = cyCharFldTtl;
    lpCnt->lpCI->cyCharFldTtl2 = cyCharFldTtl2;
    lpCnt->lpCI->cyCharFldTtl4 = cyCharFldTtl4;

   /*-----------------------------------------------------------------------*
    * Setup metrics for the general container font.
    *-----------------------------------------------------------------------*/

    // select the general container font if one has been defined
    if( lpCnt->lpCI->hFont )
      SelectObject( hDC, lpCnt->lpCI->hFont );
    else
      SelectObject( hDC, GetStockObject( lpCnt->nDefaultFont ) );

    // Get the font char width and height.
    GetTextMetrics( hDC, &tm );
    GetTextExtentPoint( hDC, "0", 1, &Size );
    cxChar = Size.cx - tm.tmOverhang;
/*    cxChar = LOWORD(GetTextExtent( hDC, "0", 1 ));*/
    cyChar = tm.tmHeight + tm.tmExternalLeading;

    // Set half and quarter char width and height dimensions.
    cxChar2 = (cxChar+1)  >> 1;
    cxChar4 = (cxChar2+1) >> 1;
    cyChar2 = (cyChar+1)  >> 1;
    cyChar4 = (cyChar2+1) >> 1;

    // set the line spacing value
    if( lpCnt->lpCI->wLineSpacing == CA_LS_NARROW )
      cyLineSpace = cyChar4;
    else if( lpCnt->lpCI->wLineSpacing == CA_LS_MEDIUM )
      cyLineSpace = cyChar2;
    else if( lpCnt->lpCI->wLineSpacing == CA_LS_WIDE )
      cyLineSpace = cyChar2 + cyChar4;
    else if( lpCnt->lpCI->wLineSpacing == CA_LS_DOUBLE )
      cyLineSpace = cyChar;
    else
      cyLineSpace = 0;                // CA_LS_NONE   

    // setup width and height of icons for name and icon views
    if (lpCnt->iView & CV_NAME || lpCnt->iView & CV_ICON)
    {
      if (lpCnt->iView & CV_MINI)
      {
#ifdef WIN32
        lpCnt->lpCI->ptBmpOrIcon.x = GetSystemMetrics(SM_CXSMICON);
        lpCnt->lpCI->ptBmpOrIcon.y = GetSystemMetrics(SM_CYSMICON);
#else
        lpCnt->lpCI->ptBmpOrIcon.x = GetSystemMetrics(SM_CXICON)/2;
        lpCnt->lpCI->ptBmpOrIcon.y = GetSystemMetrics(SM_CYICON)/2;
#endif
      }
      else
      {
        lpCnt->lpCI->ptBmpOrIcon.x = GetSystemMetrics(SM_CXICON);
        lpCnt->lpCI->ptBmpOrIcon.y = GetSystemMetrics(SM_CYICON);
      }
    }
    else
    {
      lpCnt->lpCI->ptBmpOrIcon.x = 0;
      lpCnt->lpCI->ptBmpOrIcon.y = 0;
    }

    // Calc the height of a data row.
    if( lpCnt->lpCI->wRowLines )     // user set height in char multiples
    {
      lpCnt->lpCI->cyRow = max((int)(lpCnt->lpCI->wRowLines * cyChar), lpCnt->lpCI->ptBmpOrIcon.y) +
                           cyLineSpace;
    }
    cyRow = max(lpCnt->lpCI->cyRow, lpCnt->lpCI->ptBmpOrIcon.y);

    // If no value has been set yet default to 1 char height.
    if( !cyRow )
      cyRow = max(cyChar + cyLineSpace, lpCnt->lpCI->ptBmpOrIcon.y);

    // Set half and quarter row height dimensions.
    cyRow2 = (cyRow+1)  >> 1;
    cyRow4 = (cyRow2+1) >> 1;

    // Save these dimensions in the container info struct.
    lpCnt->lpCI->cxChar  = cxChar;
    lpCnt->lpCI->cxChar2 = cxChar2;
    lpCnt->lpCI->cxChar4 = cxChar4;
    lpCnt->lpCI->cyChar  = cyChar;
    lpCnt->lpCI->cyChar2 = cyChar2;
    lpCnt->lpCI->cyRow   = cyRow;
    lpCnt->lpCI->cyRow2  = cyRow2;
    lpCnt->lpCI->cyRow4  = cyRow4;
    lpCnt->lpCI->cyLineSpace = cyLineSpace;

    // Calc the area being used for the title.
    if( lpCnt->lpCI->wTitleLines )
      lpCnt->lpCI->cyTitleArea = lpCnt->lpCI->wTitleLines * cyCharTtl + cyCharTtl2;
    cyTitleArea = lpCnt->lpCI->cyTitleArea;

   /*-----------------------------------------------------------------------*
    * Setup scrolling values.
    *-----------------------------------------------------------------------*/

    // Calc the max scrollable pixels in horz direction. 
    // maxwidth calculated in CreateFlowColStruct for text view and name view
    if (lpCnt->iView & CV_DETAIL)
    {
      lpCol = lpCnt->lpCI->lpFieldHead;  // point at 1st column
      while( lpCol )
      {
        // calc width of the column
      // nMaxWidth += lpCol->cxWidth * cxChar;
        lpCol->cxPxlWidth = lpCol->cxWidth * cxChar;
        nMaxWidth += lpCol->cxPxlWidth;
        nCols++;                     // count the nbr of fields
        lpCol = lpCol->lpNext;       // advance to next column
      }

      lpCnt->lpCI->nMaxWidth = nMaxWidth;
      lpCnt->lpCI->nFieldNbr = nCols;
    }
    else if (lpCnt->iView & CV_ICON)
    {
      CalculateIconWorkspace( hWnd );
    }

    // Set the focus to the 1st field if it hasn't been set yet.
    if (lpCnt->iView & CV_DETAIL)
    {
      if( !(lpCnt->lpCI->lpFocusFld) )
      {
        lpCnt->lpCI->lpFocusFld = lpCnt->lpCI->lpFieldHead;
        if( lpCnt->lpCI->lpFocusFld )
          lpCnt->lpCI->lpFocusFld->flColAttr |= CFA_CURSORED;
      }
    }

    // Get the container client area.
    cxClient = lpCnt->cxClient;
    cyClient = lpCnt->cyClient;

    // Get the width of a vert scroll.
    cxVscrollBar = GetSystemMetrics( SM_CXVSCROLL );

    // Calc the area being used for the column titles.
    if( lpCnt->lpCI->wColTitleLines && (lpCnt->iView & CV_DETAIL) )
      lpCnt->lpCI->cyColTitleArea = lpCnt->lpCI->wColTitleLines * cyCharFldTtl + cyCharFldTtl2;
    else
      lpCnt->lpCI->cyColTitleArea = 0;
    cyColTitleArea = lpCnt->lpCI->cyColTitleArea;

    // set the nbr of records which are in view
    // set up the linked list of columns for text view
    cyRecordArea = cyClient - cyTitleArea - cyColTitleArea;
    lpCnt->lpCI->nRecsDisplayed = (cyRecordArea-1) / cyRow + 1;

    // if flowed, don't want partial records at the bottom
    if (lpCnt->iView & CV_FLOW && lpCnt->lpCI->nRecsDisplayed > 1) 
      lpCnt->lpCI->nRecsDisplayed -= 1;

    // Create column structure used in text view and name view
    if (lpCnt->iView & CV_TEXT || lpCnt->iView & CV_NAME) 
      CreateFlowColStruct( hWnd, lpCnt );

    // Setup the horz scrollbar.
    if( lpCnt->dwStyle & CTS_HORZSCROLL )
    {
      if (lpCnt->iView & CV_ICON)
      {
        // the max scrolling range is the number of pixels the icon workspace extends
        // past the viewspace to the left plus to the right
        int nLeftPxls, nRightPxls;

        nLeftPxls  = max(0, lpCnt->lpCI->rcViewSpace.left  - lpCnt->lpCI->rcWorkSpace.left);
        nRightPxls = max(0, lpCnt->lpCI->rcWorkSpace.right - lpCnt->lpCI->rcViewSpace.right);
        lpCnt->nHscrollMax = nLeftPxls + nRightPxls;
        lpCnt->nHscrollPos = nLeftPxls;
      }
      else
      {
//      lpCnt->nHscrollMax = max(0, 2 + (nMaxWidth + cxVscrollBar - cxClient) / (int)cxChar);
        if( lpCnt->lpCI->nMaxWidth - cxClient > 0 )
          lpCnt->nHscrollMax = max(0, (lpCnt->lpCI->nMaxWidth - cxClient - 1) / lpCnt->lpCI->cxChar + 1);
        else
          lpCnt->nHscrollMax = 0;
        lpCnt->nHscrollPos = min(lpCnt->nHscrollPos, lpCnt->nHscrollMax);
      }

      SetScrollPos  ( hWnd, SB_HORZ,    lpCnt->nHscrollPos, FALSE );
      SetScrollRange( hWnd, SB_HORZ, 0, lpCnt->nHscrollMax, TRUE  );
    }

    // There is nothing to the right of this value.
    if ( !(lpCnt->iView & CV_ICON ) )
      lpCnt->xLastCol = lpCnt->lpCI->nMaxWidth - lpCnt->nHscrollPos * lpCnt->lpCI->cxChar;
 
    // Setup the vert scrollbar.
    if( lpCnt->dwStyle & CTS_VERTSCROLL )
    {
      AdjustVertScroll( hWnd, lpCnt, FALSE );
    }

   /*-----------------------------------------------------------------------*
    * All done so release the DC.
    *-----------------------------------------------------------------------*/

    ReleaseDC( hWnd, hDC );

    // Check the container size again after setting all the metrics.
    if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
          (!(lpCnt->iView & CV_ICON)) )
    {
      GetWindowRect( lpCnt->hWndFrame, &rect );
      cxWidth  = rect.right  - rect.left;
      cyHeight = rect.bottom - rect.top;

      // If the container adjusted its width or height to stay on integral
      // boundaries tell the associate wnd about the size change.
      if( cxWidth != cxOldWidth || cyHeight != cyOldHeight )
        NotifyAssocWnd( hWnd, lpCnt, CN_SIZECHANGED, 0, NULL, NULL, 0, 0, 0, NULL );
    }

    return;
}

/****************************************************************************
 * AdjustVertScroll
 *
 * Purpose:
 *  Calculates the vertical scroll range, position, and pointer to the 
 *  1st record being displayed at the top of the screen.
 *  Should be called whenever the size, font, or number of records change.
 *
 * Parameter:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *  BOOL          bSizing     - flag to indicate we are processing a WM_SIZE msg
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI AdjustVertScroll( HWND hWnd, LPCONTAINER lpCnt, BOOL bSizing )
{
    LPCONTAINER lpCntFrame;
    RECT  rect;
    POINT pt;
    int   cxWidth, cyHeight;
    int   cyRecordArea;
    int   nOldMax;
    BOOL  bUpdateScroll;

    if( lpCnt->dwStyle & CTS_VERTSCROLL )
    {
      // Calc the area being used for record data.
      if (lpCnt->iView & CV_DETAIL)
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea - lpCnt->lpCI->cyColTitleArea;
      else
        cyRecordArea = lpCnt->cyClient - lpCnt->lpCI->cyTitleArea;

      nOldMax = lpCnt->nVscrollMax;
      if ( lpCnt->iView & CV_ICON )
      {
        // the max scrolling range is the number of pixels the icon workspace extends
        // past the viewspace from the top plus past the bottom
        int nTopPxls, nBottomPxls;

        if (lpCnt->lpCI->bArranged)
          nTopPxls = max(0, lpCnt->lpCI->rcViewSpace.top - 
                           (lpCnt->lpCI->rcWorkSpace.top - lpCnt->lpCI->aiInfo.yIndent));
        else  
          nTopPxls = max(0, lpCnt->lpCI->rcViewSpace.top - lpCnt->lpCI->rcWorkSpace.top);
        nBottomPxls = max(0, lpCnt->lpCI->rcWorkSpace.bottom - lpCnt->lpCI->rcViewSpace.bottom);
        lpCnt->nVscrollMax = nTopPxls + nBottomPxls;
        lpCnt->nVscrollPos = nTopPxls;
      }
      else
      {
        if (lpCnt->iView & CV_FLOW)
        {
          lpCnt->nVscrollMax = 0; // no vertical scroll bar for flowed view.
        }
        else
          lpCnt->nVscrollMax = max(0, lpCnt->iMax - cyRecordArea / lpCnt->lpCI->cyRow);
        lpCnt->nVscrollPos = min(lpCnt->nVscrollPos, lpCnt->nVscrollMax);
      }

      // Only repaint the vertical scrollbar if the max value has changed.
      bUpdateScroll = (nOldMax == lpCnt->nVscrollMax) ? FALSE : TRUE;

      // Only the 1st child shows a vertical scrollbar.
      if( lpCnt->dwStyle & CTS_SPLITBAR )
      {
        SetScrollPos( hWnd, SB_VERT,    lpCnt->nVscrollPos, FALSE );
        if( hWnd == lpCnt->hWnd1stChild )
          SetScrollRange( hWnd, SB_VERT, 0, lpCnt->nVscrollMax, bUpdateScroll );
        else
          SetScrollRange( hWnd, SB_VERT, 0, 0, FALSE );
      }
      else
      {
#ifdef WIN32
        SCROLLINFO ScrInfo;
        
        ScrInfo.fMask  = SIF_RANGE | SIF_POS;
        ScrInfo.nMin   = 0;
        ScrInfo.nMax   = lpCnt->nVscrollMax;
        ScrInfo.nPos   = lpCnt->nVscrollPos;
        ScrInfo.cbSize = sizeof(ScrInfo);
        if (lpCnt->lpCI->flCntAttr & CA_SHVERTSCROLL)
          ScrInfo.fMask |= SIF_DISABLENOSCROLL; // always show the vertical scroll bar;

        SetScrollInfo( hWnd, SB_VERT, &ScrInfo, bUpdateScroll );
#else
        SetScrollPos  ( hWnd, SB_VERT,    lpCnt->nVscrollPos, FALSE  );
        SetScrollRange( hWnd, SB_VERT, 0, lpCnt->nVscrollMax, bUpdateScroll );
#endif
      }

      // Scroll the record list to the 1st record displayed if not owner scroll.
      if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) && !(lpCnt->iView & CV_ICON) )
      {
        if( !lpCnt->lDeltaPos )
          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpRecHead, lpCnt->nVscrollPos );
        else
          lpCnt->lpCI->lpTopRec = CntScrollRecList( hWnd, lpCnt->lpCI->lpRecHead, lpCnt->nVscrollPos-lpCnt->lDeltaPos );
      }
      else
      {
        if ((lpCnt->iView & CV_ICON) && lpCnt->lpCI->lpRecOrderHead)
          lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecOrderHead->lpRec;
        else if (lpCnt->iView & CV_ICON)
          lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecHead;
      }
    }
    else
    {
      // If there is no vert scrollbar the 1st record displayed is the head.
      if ((lpCnt->iView & CV_ICON) && lpCnt->lpCI->lpRecOrderHead)
        lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecOrderHead->lpRec;
      else
        lpCnt->lpCI->lpTopRec = lpCnt->lpCI->lpRecHead;
    }

    // Set the focus to the 1st record displayed if it hasn't been set yet.
    if( !(lpCnt->lpCI->lpFocusRec) && !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL) )
    {
      LRESULT lResult = 0L;
      
      // notify the user we are about to set the focus - if he returns <> 0 then
      // we do not set the focus.  This for cases like the delta model where the
      // focus has been swapped out of memory.
      lResult = NotifyAssocWndEx( hWnd, lpCnt, CN_DONTSETFOCUSREC, 0, NULL, NULL, 0, 0, 0, NULL );
      if (lResult==0)
      {
        lpCnt->lpCI->lpFocusRec = lpCnt->lpCI->lpTopRec;
        if( lpCnt->lpCI->lpFocusRec )
          lpCnt->lpCI->lpFocusRec->flRecAttr |= CRA_CURSORED;
      }
    }

    // Force a WM_SIZE with MoveWindow to adjust the width or height if the
    // integral styles are enabled (if we not already doing a WM_SIZE).
    if( !bSizing && !lpCnt->bIsSplit )
    {
      if( ((lpCnt->dwStyle & CTS_INTEGRALWIDTH) || (lpCnt->dwStyle & CTS_INTEGRALHEIGHT)) &&
         (!(lpCnt->iView & CV_ICON)) )
      {
        lpCntFrame = GETCNT( lpCnt->hWndFrame );
        GetWindowRect( lpCnt->hWndFrame, &rect );

        pt.x = rect.left;
        pt.y = rect.top;
        ScreenToClient( GetParent(lpCnt->hWndFrame), &pt );

        // Use the original user-set size so we don't keep shrinking.
        cxWidth  = lpCntFrame->cxWndOrig;
        cyHeight = lpCntFrame->cyWndOrig;

        // Don't do any moving until base width and height are initialized.
        // Also, don't do this if we are in the middle of moving a field.
        // If would just cause an unnecessary flash as the field is first
        // removed from the list and then reinserted.
        if( cxWidth && cyHeight && !lpCntFrame->bMovingFld )
        {
          // Turn on flag so base dimensions aren't chged in WM_SIZE.
          lpCntFrame->bIntegralSizeChg = TRUE;

          MoveWindow( lpCnt->hWndFrame, pt.x, pt.y, cxWidth, cyHeight, TRUE );

          lpCntFrame->bIntegralSizeChg = FALSE;
        }
      }
    }
}

/****************************************************************************
 * UpdateContainer
 *
 * Purpose:
 *  Called when the container needs to be repainted.
 *  Will not repaint if container is in the deferred paint state.
 *
 * Parameters:
 *  HWND          hWnd         - handle to the container window
 *  LPRECT        lpRect       - rect to update
 *  BOOL          bEraseBkGrnd - flag to erase background or not
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI UpdateContainer( HWND hWnd, LPRECT lpRect, BOOL bEraseBkGrnd )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);

    if( !StateTest( lpCnt, CNTSTATE_DEFERPAINT ) )
      {
      InvalidateRect( hWnd, lpRect, bEraseBkGrnd );
      UpdateWindow( hWnd );
      }
    }

/****************************************************************************
 * ContainerDestroy
 *
 * Purpose:
 *  Frees all memory associated with the container.
 *  Called when the container receives the WM_DESTROY message.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  BOOL          0 if memory was freed without problem; 1 if error
 ****************************************************************************/

BOOL WINAPI ContainerDestroy( HWND hWnd, LPCONTAINER lpCnt )
    {
    BOOL bStatus=0;

    // Free memory for the field list, record list, and cursors only if
    // this is the 1st split child (means container is closing).
    if( hWnd == lpCnt->hWnd1stChild )
      {
      CntKillFldList( hWnd );
      CntKillRecList( hWnd );

      // Select this so we don't try to delete a cursor in use.
      SetCursor( LoadCursor( NULL, IDC_ARROW ) );
    
      // Free the default font.
      if( lpCnt->hFontDefault )
        {
        DeleteObject( lpCnt->hFontDefault );
        lpCnt->hFontDefault = NULL;
        }

      // Free extra cursors.
      DestroyCursor( lpCnt->lpCI->hCurFldSizing );
      DestroyCursor( lpCnt->lpCI->hCurFldMoving );

      // We can't destroy this one because it is the container class cursor.
/*      DestroyCursor( lpCnt->hCurSplitBar );*/

      // Delete the container color objects.
      CntColorManager( hWnd, lpCnt, CLR_DELETE );

      // Free any title data stored in the CNTINFO struct.
      if( lpCnt->lpCI->lpszTitle )
/*        GlobalFreePtr( lpCnt->lpCI->lpszTitle );*/
        free( lpCnt->lpCI->lpszTitle );

      // Free any user data stored in the CNTINFO struct.
      if( lpCnt->lpCI->lpUserData )
/*        GlobalFreePtr(lpCnt->lpCI->lpUserData);*/
        free(lpCnt->lpCI->lpUserData);

      // Free global memory for the CNTINFO struct.
/*      GlobalFreePtr( lpCnt->lpCI );*/
      free( lpCnt->lpCI );
      lpCnt->lpCI = NULL;

      // Free global memory for the CONTAINER struct.
/*      GlobalFreePtr( lpCnt );*/
      free( lpCnt );
      lpCnt = NULL;

      // Null out the container pointer in the window extra bytes.
      SetWindowLong( hWnd, GWL_CONTAINERHMEM, (LONG)lpCnt );
      }
    else
      {
      // Free global memory for the CONTAINER struct.
/*      GlobalFreePtr( lpCnt );*/
      free( lpCnt );
      lpCnt = NULL;

      // Null out the container pointer in the window extra bytes.
      SetWindowLong( hWnd, GWL_CONTAINERHMEM, (LONG)lpCnt );
      }

    return bStatus;
    }

/****************************************************************************
 * IsFocusInView
 *
 * Purpose:
 *  Determines whether the focus cell is viewable in the display area.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *
 * Return Value:
 *  BOOL          TRUE if any part of the focus cell is viewable
 *                FALSE if no part of the focus cell is viewable
 ****************************************************************************/

BOOL WINAPI IsFocusInView( HWND hWnd, LPCONTAINER lpCnt )
    {
    BOOL bIsInView=FALSE;
    BOOL bFldInView;

    if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      // First check out the focus field. When using wide focus rectangle
      // we say the focus field is always in view. This prevents funny
      // snap-back-to-focus-cell behavior.
      if( !lpCnt->bWideFocusRect )
        bFldInView = IsFldInView( hWnd, lpCnt, lpCnt->lpCI->lpFocusFld );
      else
        bFldInView = TRUE;

      if( bFldInView )
        {
        // If the field is in view check out the focus record.
        if( IsRecInView( hWnd, lpCnt, lpCnt->lpCI->lpFocusRec ) )
          bIsInView = TRUE;
        }
      }

    return bIsInView;
    }

/****************************************************************************
 * ScrollToFocus
 *
 * Purpose:
 *  Scrolls the focus cell into view. Is called if the focus cell is not
 *  in view and a keyboard char is pressed. The scrolling should occur
 *  prior to the desired keyboard action.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the container window
 *  LPCONTAINER   lpCnt       - pointer to container structure
 *  UINT          wVertDir    - vert direction user is scrolling: 0=UP; 1=DOWN
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI ScrollToFocus( HWND hWnd, LPCONTAINER lpCnt, UINT wVertDir )
    {
    LPCONTAINER  lpCntFrame = GETCNT( lpCnt->hWndFrame );
    LPRECORDCORE lpRec;
    BOOL         bFound=FALSE;
    int          nInc=0;
    int          xStrCol, xEndCol;

    // If the scroll-to-focus behavior is disabled just return.
    if( !lpCntFrame->bScrollFocusRect )
      return;

    // First look for the focus record in the list.
    if( lpCnt->lpCI->lpFocusRec && lpCnt->lpCI->lpFocusFld )
      {
      // If app is handling the vertical scroll, don't look for it.
      if( !(lpCnt->lpCI->flCntAttr & CA_OWNERVSCROLL ) )
        {
        lpRec = CntRecHeadGet( hWnd );
        while( lpRec )
          {
          if( lpRec == lpCnt->lpCI->lpFocusRec )
            {
            bFound = TRUE;
            break;
            }

          lpRec = lpRec->lpNext;
          nInc++;
          }
        }
      }

    // Scroll vertically to the focus record if it was found.
    if( bFound )
      {
      // NOTE: This never happens if CA_OWNERVSCROLL is active!
      nInc -= wVertDir ? (lpCnt->lpCI->nRecsDisplayed-2) : 1;

      // Note that for 32 bit scrolling we are passing the position in the
      // unused hwndCtl parm slot. When the msg is received that value will
      // be used instead of trying to calculate the position.
      if( StateTest( lpCnt, CNTSTATE_VSCROLL32 ) )
        FORWARD_WM_VSCROLL( hWnd, nInc, SB_THUMBPOSITION, nInc, SendMessage );
      else
        FORWARD_WM_VSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
      }
    else
      {
      // If not found it means CA_OWNERVSCROLL is active or we are in a
      // DELTA model and the focus record is not currently in memory.
      // In either case the app knows where the focus record is and will
      // build a new list with the focus record at the head.
      NotifyAssocWnd( hWnd, lpCnt, CN_QUERYFOCUS, 0, NULL, NULL, 0, 0, 0, NULL );
      }

    // Now scroll horizontally to the focus field (if not using wide focus).
/*    if( lpCnt->lpCI->lpFocusFld )*/
    if( lpCnt->lpCI->lpFocusFld && !lpCnt->bWideFocusRect )
      {
      GetFieldXPos( lpCnt, lpCnt->lpCI->lpFocusFld, &xStrCol, &xEndCol );

      if( xStrCol < 0 || xStrCol > lpCnt->cxClient )
        {
        // Focus rect was not in display area or it is now moving
        // out of the display area so we will do a scroll.
        nInc = xStrCol / lpCnt->lpCI->cxChar;
        nInc += lpCnt->nHscrollPos;
        FORWARD_WM_HSCROLL( hWnd, 0, SB_THUMBPOSITION, nInc, SendMessage );
        }
      }

    return;
    }

/****************************************************************************
 * CntColorManager
 *
 * Purpose:
 *  Initializes, updates, or deletes the container's color list and
 *  the associated pens.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control
 *  LPCONTAINER   lpCnt       - control data pointer
 *  UINT          wAction     - type of action to execute
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID WINAPI CntColorManager( HWND hWnd, LPCONTAINER lpCnt, UINT wAction )
    {  
    HDC hDC;
    int iColor;

    if( wAction == CLR_UPDATE || wAction == CLR_DELETE )
      {
      hDC = GetDC( hWnd );

      // Select this so we don't try to delete a selected pen.
      SelectObject( hDC, GetStockObject( BLACK_PEN ) );

      // Delete this container's pens.
      for( iColor=0; iColor < CNTCOLORS; iColor++ )
        DeleteObject( lpCnt->hPen[iColor] );

      ReleaseDC( hWnd, hDC );
      }

    if( wAction == CLR_INIT )
      {
      // Load up the system default colors.
      for( iColor=0; iColor < CNTCOLORS; iColor++ )
        lpCnt->crColor[iColor] = GetSysColor( crDefColor[iColor] );
      }

    if( wAction == CLR_INIT || wAction == CLR_UPDATE )
      {
      // Create all the container pens.  
      for( iColor=0; iColor < CNTCOLORS; iColor++ )
        lpCnt->hPen[iColor] = CreatePen( PS_SOLID, 1, lpCnt->crColor[iColor] );
      }

    return;
    }

COLORREF WINAPI GetCntDefColor( UINT iColor )
    {
    if( iColor >= CNTCOLORS )
      return 0L;

    return GetSysColor( crDefColor[iColor] );
    }

/****************************************************************************
 * CntTimerProc
 *
 * Purpose:
 *  Handles timer needs. Called at each timeout.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the timer window
 *  UINT          msg         - WM_TIMER message
 *  UINT          idTimer     - timer identifier
 *  DWORD         dwTime      - current system time
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID CALLBACK CntTimerProc( HWND hWnd, UINT msg, UINT idTimer, DWORD dwTime )
    {
    LPCONTAINER lpCnt = GETCNT(hWnd);
    POINT      pt;

    if( idTimer == IDT_AUTOSCROLL && StateTest( lpCnt, CNTSTATE_CLICKED) )
      {
      GetCursorPos( &pt );
      ScreenToClient( hWnd, &pt );

      // Post a MOUSEMOVE if the cursor is in the auto-scroll area.
      if( !PtInRect( &lpCnt->rectRecArea, pt ) )
        {
        PostMessage( hWnd, WM_MOUSEMOVE, 0, MAKELONG(pt.x, pt.y) );
        }
      }

    return;
    }

/****************************************************************************
 * CreateDefaultFont
 *
 * Purpose:
 *  Creates the container default font.
 *
 * Parameters:
 *  HWND          hWnd        - handle to the control
 *  LPCONTAINER   lpCnt       - control data pointer
 *
 * Return Value:
 *  VOID
 ****************************************************************************/

VOID NEAR CreateDefaultFont( HWND hWnd, LPCONTAINER lpCnt )
    {  
    HDC     hDC;
    LOGFONT lfLogFont;
    char    szTmp[128];   
    char    szFaceName[128];   
    int     nHeight;
    int     nLogPixY;
    int     nPointSize;  
    int     nPitch;
    int     nFamily;
    int     nWeight;
    int     nCharSet;

    // Get the font specs from the resource file.
    lpCnt->nDefaultFont = GetResStr2Int( IDS_FONT_DEFAULT, szTmp, sizeof(szTmp) );
    GetResStr( IDS_FONT_FACENAME, szFaceName, sizeof(szFaceName) );
    nPointSize = GetResStr2Int( IDS_FONT_POINTSIZE, szTmp, sizeof(szTmp) );
    nWeight    = GetResStr2Int( IDS_FONT_WEIGHT,    szTmp, sizeof(szTmp) );
    nFamily    = GetResStr2Int( IDS_FONT_FAMILY,    szTmp, sizeof(szTmp) );
    nPitch     = GetResStr2Int( IDS_FONT_PITCH,     szTmp, sizeof(szTmp) );
    nCharSet   = GetResStr2Int( IDS_FONT_CHARSET,   szTmp, sizeof(szTmp) );

    // Convert the pointsize to pixel height.
    hDC = GetDC( hWnd );
    nLogPixY = GetDeviceCaps( hDC, LOGPIXELSY );
    nHeight  = nPointSize * nLogPixY / 72;
    ReleaseDC( hWnd, hDC );

    // Set the attributes for the desired font.
    lfLogFont.lfHeight         = -nHeight;
    lfLogFont.lfWidth          = 0;  // neg height & 0 width take care of aspect ratio
    lfLogFont.lfEscapement     = 0;
    lfLogFont.lfOrientation    = 0;
    lfLogFont.lfWeight         = nWeight;
    lfLogFont.lfItalic         = 0;
    lfLogFont.lfUnderline      = 0;
    lfLogFont.lfStrikeOut      = 0;
    lfLogFont.lfCharSet        = (BYTE)nCharSet;
    lfLogFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lfLogFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lfLogFont.lfQuality        = PROOF_QUALITY;
    lfLogFont.lfPitchAndFamily = (BYTE)(nFamily | nPitch);
    lstrcpy( lfLogFont.lfFaceName, szFaceName );

    // Create the font.
    lpCnt->hFontDefault = CreateFontIndirect( &lfLogFont );

    return;
    }

VOID NEAR GetResStr( int nResID, LPSTR lpBuf, UINT cBytes )
    {
    if( lpBuf )
      {   
      *lpBuf = '\0';
      LoadString( hInstCnt, nResID, lpBuf, cBytes );
      }
    }

int NEAR GetResStr2Int( int nResID, LPSTR lpBuf, UINT cBytes )
    {
    GetResStr( nResID, lpBuf, cBytes );
    return( atoi(lpBuf) );
    }

